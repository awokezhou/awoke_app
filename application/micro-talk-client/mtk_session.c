
#include "mtk_session.h"
#include "mtk_client.h"
#include "mtk_protocol.h"



int mtk_session_id_get(struct _mtk_context *ctx)
{
	return (++ctx->session_baseid);
}

bool mtk_session_running(struct _mtk_session *ss)
{
	return (ss->state != MTK_SESSION_NONE);
}

err_type mtk_session_start(struct _mtk_context *ctx, struct _mtk_work *work)
{
	mtk_session *ss = work->data;
	mtk_connect *conn = &ctx->connects[ss->channel];

	/* current session restart */
	if (conn->session && (ss->id == conn->session->id)) {
		log_trace("session[%d] restart", ss->id);
		return mtk_work_schedule(ctx, mtk_dc_connect, 1*MTK_US_PER_MS);
	}

	/* current session running, new session wait */
	if (conn->session && mtk_session_running(conn->session)) {
		log_trace("session[%d] is running, session[%d] wait", conn->session->id, ss->id);
		return mtk_work_schedule_withdata(ctx, mtk_session_start, 1*MTK_US_PER_SEC, ss);
	}

	/* new session can schedule now */
	log_debug("session[%d] sched", ss->id);
	mtk_connect_session_add(conn, ss);
	return mtk_work_schedule(ctx, mtk_dc_connect, 1*MTK_US_PER_MS);
}

mtk_session *mtk_session_clone(struct _mtk_session *src)
{
	mtk_session *ss = mem_alloc_trace(sizeof(mtk_session), "session");
	if (!ss) {
		log_err("alloc session error");
		return NULL;
	}
	memset(ss, 0x0, sizeof(mtk_session));

	ss->type = MTK_SESSION_DYNAMIC;
	ss->name = awoke_string_dup(src->name);
	ss->channel = src->channel;
	ss->req_process = src->req_process;
	ss->rsp_process = src->rsp_process;

	return ss;
}

void mtk_session_free(struct _mtk_session *ss)
{
	if (ss->type == MTK_SESSION_STATIC)
		return;

	if (ss->data) {
		ss->destory(ss->data);
		ss->data = NULL;
	}

	if (ss->name) {
		mem_free(ss->name);
	}

	mem_free(ss);
}

static err_type chunkrecv_request(struct _mtk_connect *c)
{
	err_type ret;
	mtk_context *ctx = c->context;
	mtk_chunkinfo *ci = c->session->data;
	mtk_request_msg msg;

	uint8_t *pos;
	uint16_t index;
	uint32_t fileid;

	/*
	 * -- GET ChunkData Request Format --
	 *
	 *  +--------------------------------+----------------+
	 *  |            fileid              |     index      |
	 *  +--------------------------------+----------------+
	 *  |             4Byte              |      2Byte     |
	 *  +--------------------------------+----------------+
	 */

	memset(&msg, 0x0, sizeof(msg));
	msg.payload = mem_alloc_trace(MTK_REQ_HEADER_LENGTH+6, "GET ChunkData Request");
	if (!msg.payload) {
		log_err("alloc Request error");
		return et_nomem;
	}

	pos = msg.payload + MTK_REQ_HEADER_LENGTH - 1;

	/* fileid */
	fileid = htonl(ci->fileid);
	pkg_push_dwrd(fileid, pos);

	/* index */
	index = htons(ci->index);
	pkg_push_word(index, pos);

	/* header */
	c->last_method = MTK_METHOD_GET;
	ret = mtk_pkg_request_header(&msg, MTK_METHOD_GET, 6, MTK_LBL_FILE, ctx->imei);
	if (ret != et_ok) {
		log_err("header error:%d", ret);
		mem_free(msg.payload);
		goto err;
	}

	log_debug("GET ChunkData Request(%d)", msg.length);

	log_trace("");
	log_trace("GET Request(%d)", msg.length);
	log_trace("----------------");
	log_trace("fileid:%d", ci->fileid);
	log_trace("index:%d", ci->index);
	awoke_hexdump_trace(msg.payload, msg.length);
	log_trace("----------------");
	log_trace("");

	log_info("request chunk %d", ci->index);

	awoke_queue_enq(c->ring, &msg);

	mtk_callback_on_writable(c);

	c->session->state = MTK_SESSION_RSP;

	return et_ok;

err:
	mtk_connect_session_del(c);
	return ret;
}

static err_type chunkrecv_response(struct _mtk_connect *c, struct _mtk_response_msg *m)
{
	uint8_t code;
	uint16_t chunknum;
	uint16_t crc, dlen;
	uint32_t totalsize;
	err_type ret;
	mtk_session *ss = c->session;
	mtk_chunkinfo *ci = ss->data;
	mtk_context *ctx = c->context;

	uint8_t *pos = m->data;

	/*  
	 *  -- GET ChunkData Response Format --
	 *
	 *  +--------+----------------+--------------------------------+----------------+----------
	 *  |  code  |    chunknum    |           totalsize            |       CRC      | DATA
	 *  +--------+----------------+--------------------------------+----------------+----------
	 *  |  1Byte |      2Byte     |             4Byte              |      2Byte     | xByte...
	 *  +--------+----------------+--------------------------------+----------------+----------
	 */


	/* code */
	pkg_pull_byte(code, pos);

	/* chunknum */
	pkg_pull_word(chunknum, pos);
	chunknum = htons(chunknum);

	/* totalsize */
	pkg_pull_dwrd(totalsize, pos);
	totalsize = htonl(totalsize);

	/* CRC */
	pkg_pull_word(crc, pos);
	crc = htons(crc);

	if (code != 0x0) {
		log_err("error code:%d", code);
		ret = et_fail;
		goto err;
	}

	if (!ci->chunk_pq) {

		log_debug("GET ChunkData Response(%d)", m->len);

		log_trace("");
		log_trace("ChunkData Response(%d):", m->len);
		log_trace("----------------");
		log_trace("code:%d", code);
		log_trace("chunk number:%d", chunknum);
		log_trace("filesize:%d", totalsize);
		log_trace("CRC:0x%x", crc);
		log_trace("----------------");
		log_trace("");

		ci->crc = crc;
		ci->chunknum = chunknum;
		ci->totalsize = totalsize;

		ci->chunk_pq = awoke_minpq_create(sizeof(awoke_buffchunk), chunknum, NULL, 0x0);
		if (!ci->chunk_pq) {
			log_err("chunk queue create error");
			ret = et_nomem;
			goto err;
		}

		log_trace("chunk queue create(%d)", chunknum);
	}

	if (awoke_minpq_full(ci->chunk_pq)) {
		log_err("chunk queue full, recv error");
		ret = et_full;
		goto err;
	}

	dlen = m->len - 9;
	pos = m->data + 9;
	awoke_buffchunk *chunk = awoke_buffchunk_create(dlen);
	if (!chunk) {
		log_err("alloc chunkdata error");
		ret = et_nomem;
		goto err;
	}

	log_info("chunk %d length %d [%d:%d]", ci->index, dlen, chunk->length, chunk->length+dlen-1);
	ret = awoke_buffchunk_write(chunk, (const char *)pos, dlen, FALSE);
	if (ret != et_ok) {
		log_err("chunkdata write error");
		goto err;
	}

	awoke_minpq_insert(ci->chunk_pq, chunk, ci->index);
	mem_free(chunk);

	if (!awoke_minpq_full(ci->chunk_pq)) {
		log_trace("file %d recv not finish", ci->fileid);
		ci->index++;
		ss->state = MTK_SESSION_REQ;
		return mtk_work_schedule_withdata(ctx, mtk_session_start, 1*MTK_US_PER_MS, ss);
	}

	log_info("get file %d finish", ci->fileid);
	ret = mtk_chunkdata_merge(ci);
	if (ret != et_ok) {
		log_err("chunkdata merge error:%d", ret);
		goto err;
	}

	log_debug("session[%d] FIN", ss->id);
	ss->state = MTK_SESSION_FIN;

	return et_ok;

err:
	mtk_connect_session_del(c);
	return ret;
}

err_type chunksend_request(struct _mtk_connect *c)
{
	err_type ret;
	mtk_context *ctx = c->context;
	mtk_chunkinfo *ci = c->session->data;
	mtk_request_msg msg;
	int header_length = 12;

	int dlen;
	uint8_t *pos;
	uint16_t crc;
	uint16_t hash;
	uint16_t index;
	uint16_t chunknum;
	uint32_t start;
	uint32_t totalsize;

	/*
	 * -- PUT ChunkData Request Format --
	 *
	 *  +----------------+----------------+--------------------------------+----------------+----------------+------
	 *  |    chunknum    |     index      |            totalsize           |       CRC      |       HASH     | DATA
	 *  +----------------+----------------+--------------------------------+----------------+----------------+------
	 *  |     2Byte      |     2Byte      |              4Byte             |      2Byte     |       2Byte    |
	 *  +----------------+----------------+--------------------------------+----------------+----------------+------
	 */

	memset(&msg, 0x0, sizeof(msg));
	msg.payload = mem_alloc_trace(MTK_REQ_HEADER_LENGTH+header_length+ci->chunksize, "PUT ChunkData Request");
	if (!msg.payload) {
		log_err("alloc buffer error");
		return et_nomem;
	}

	pos = msg.payload + MTK_REQ_HEADER_LENGTH - 1;

	/* chunknum */
	chunknum = htons(ci->chunknum);
	pkg_push_word(chunknum, pos);

	/* index */
	index = htons(ci->index);
	pkg_push_word(index, pos);

	/* totalsize */
	totalsize = htonl(ci->totalsize);
	pkg_push_dwrd(totalsize, pos);

	/* CRC */
	crc = htons(ci->crc);
	pkg_push_word(crc, pos);

	/* HASH */
	hash = htons(ci->hash);
	pkg_push_word(hash, pos);

	/* chunkdata */
	start = (ci->index == 0) ? 0 : ci->index*ci->chunksize;
	dlen = (ci->index == (ci->chunknum-1)) ? (ci->totalsize-start) : ci->chunksize;
	ret = mtk_chunkdata_copy(ci, pos, start, dlen);
	if (ret != et_ok) {
		log_err("copy error:%d", ret);
		goto err;
	}
	log_info("chunk:%d [%llu:%llu] size:%d", ci->index, start, 
		start+dlen-1, dlen);
	
	/* header */
	c->last_method = MTK_METHOD_PUT;
	ret = mtk_pkg_request_header(&msg, MTK_METHOD_PUT, header_length+dlen, 
		MTK_LBL_FILE, ctx->imei);
	if (ret != et_ok) {
		log_err("header error:%d", ret);
		mem_free(msg.payload);
		goto err;
	}

	log_info("PUT ChunkData Request(%d)", msg.length);

	awoke_queue_enq(c->ring, &msg);

	mtk_callback_on_writable(c);

	awoke_hexdump_trace(msg.payload, 32);

	c->session->state = MTK_SESSION_RSP;

	return et_ok;

err:
	mtk_connect_session_del(c);
	return ret;
}

static err_type chunksend_response(struct _mtk_connect *c, struct _mtk_response_msg *m)
{
	uint8_t code;
	uint32_t fileid;
	mtk_session *ss = c->session;
	mtk_chunkinfo *ci = ss->data;
	mtk_context *ctx = c->context;

	uint8_t *pos = m->data;
	
	/*  
	 *  -- PUT ChunkData Response Format --
	 *
	 *  +--------+----------------+
	 *  |  code  |     fileid     |
	 *  +--------+----------------+
	 *  |  1Byte |      2Byte     |
	 *  +--------+----------------+
	 */

	/* code */
	pkg_pull_byte(code, pos);

	/* fileid */
	pkg_pull_dwrd(fileid, pos);
	fileid = htonl(fileid);

	log_debug("code:%d fileid:%d", code, fileid);

	if ((ci->index == (ci->chunknum-1)) && (code != 0x1)) {
		log_notice("there maybe some error on server");
		ss->state = MTK_SESSION_FIN;
		return et_ok;
	}

	if (code == 0x3) {
		log_err("checksum error");
		ss->state = MTK_SESSION_FIN;
		return et_fail;
	} else if (code == 0x04) {
		log_err("transmit fail");
		ss->state = MTK_SESSION_FIN;
		return et_fail;
	} else if (code == 0x0) {
		log_info("index:%d transmit success", ci->index);
		ci->index++;
		ss->state = MTK_SESSION_REQ;
		return mtk_work_schedule_withdata(ctx, mtk_session_start, 1*MTK_US_PER_MS, ss);
	} else {
		log_info("send file %s finish", ci->filepath);
		log_debug("session[%d] FIN", ss->id);
		ss->state = MTK_SESSION_FIN;
		c->last_fileid = fileid;
		log_debug("record last fileid %d");
	}

	return et_ok;
}

static err_type getcmd_request(struct _mtk_connect *c)
{
	err_type ret;
	mtk_context *ctx = c->context;
	mtk_request_msg msg;

	/*
	 * GET Command Request has no DAT
	 */

	memset(&msg, 0x0, sizeof(msg));
	msg.payload = mem_alloc_trace(MTK_REQ_HEADER_LENGTH, "GET Command Request");
	if (!msg.payload) {
		log_err("alloc Request error");
		return et_nomem;
	}

	/* header */
	c->last_method = MTK_METHOD_GET;
	ret = mtk_pkg_request_header(&msg, MTK_METHOD_GET, 0, MTK_LBL_COMMAND, ctx->imei);
	if (ret != et_ok) {
		log_err("header error:%d", ret);
		mem_free(msg.payload);
		return ret;
	}

	log_debug("GET Command Request(%d)", msg.length);

	log_trace("");
	log_trace("GET Command Request(%d)", msg.length);
	log_trace("----------------");
	awoke_hexdump_trace(msg.payload, msg.length);
	log_trace("----------------");
	log_trace("");

	awoke_queue_enq(c->ring, &msg);

	mtk_callback_on_writable(c);

	c->session->state = MTK_SESSION_RSP;

	return et_ok;	
}

static err_type getcmd_response(struct _mtk_connect *c, struct _mtk_response_msg *m)
{
	uint8_t cmdtype;
	uint32_t fileid;
	uint8_t *pos = m->data;
	char filepath[32] = {'\0'};

	log_debug("record last fileid %d", c->last_fileid);
	
	/*  
	 *  -- GET Command Response Format --
	 *
	 *  +--------------------------------+--------+
	 *  |             fileid             | cmdtype|
	 *  +--------------------------------+--------+
	 *  |             4Byte              |  1Byte |
	 *  +--------------------------------+--------+
	 */

	
	log_debug("GET Command Response(%d)", m->len);

	if (!m->len) {
		log_debug("no command");
		return et_ok;
	}

	/* fileid */
	pkg_pull_dwrd(fileid, pos);
	fileid = htonl(fileid);

	/* cmdtype */
	pkg_pull_byte(cmdtype, pos);

	if (fileid && (fileid != c->last_fileid)) {
		log_trace("fileid:%d, cmdtype:%d", fileid, cmdtype);
		log_info("file %d need download", fileid);
		sprintf(filepath, "test-recv-%d.amr", fileid);
		mtk_voicefile_recv(c->context, filepath, fileid);
	} else {
		log_info("no file need to download");
	}
	
	c->session->state = MTK_SESSION_FIN;

	return et_ok;
}

mtk_session chunkrecv = {
	.id = 0,
	.name = "chunkrecv",
	.req_process = chunkrecv_request,
	.rsp_process = chunkrecv_response,
	.type = MTK_SESSION_STATIC,
	.channel = MTK_CHANNEL_DATA,
	.state = MTK_SESSION_NONE,
};

mtk_session chunksend = {
	.id = 0,
	.name = "chunksend",
	.req_process = chunksend_request,
	.rsp_process = chunksend_response,
	.type = MTK_SESSION_STATIC,
	.channel = MTK_CHANNEL_DATA,
	.state = MTK_SESSION_NONE,
};

mtk_session getcmd = {
	.id = 0,
	.name = "getcmd",
	.req_process = getcmd_request,
	.rsp_process = getcmd_response,
	.type = MTK_SESSION_STATIC,
	.channel = MTK_CHANNEL_DATA,
	.state = MTK_SESSION_NONE,
};

err_type mtk_session_add(struct _mtk_context *ctx, struct _mtk_session *clone, void *data, 
	void (*destory)(void *))
{
	mtk_session *ss;
	ss = mtk_session_clone(clone);
	ss->id = mtk_session_id_get(ctx);
	ss->data = data;
	ss->state = MTK_SESSION_REQ;
	ss->destory = destory;

	log_debug("session[%d] create", ss->id);

	return mtk_work_schedule_withdata(ctx, mtk_session_start, 1*MTK_US_PER_SEC, ss);
}

err_type mtk_voicefile_recv(struct _mtk_context *ctx, const char *filepath, uint32_t fileid)
{
	mtk_session *ss;
	mtk_chunkinfo *ci;

	ci = mem_alloc_trace(sizeof(mtk_chunkinfo), "chunkinfo");
	if (!ci) {
		log_err("alloc chunkinfo error");
		return et_nomem;
	}
	memset(ci, 0x0, sizeof(mtk_chunkinfo));
	ci->index = 0;
	ci->fileid = fileid;
	if (filepath)
		ci->filepath = awoke_string_dup(filepath);
	ci->type = MTK_CHUNKTYPE_FILE;

	ss = mtk_session_clone(&chunkrecv);
	ss->id = mtk_session_id_get(ctx);
	ss->data = ci;
	ss->state = MTK_SESSION_REQ;
	ss->destory = mtk_chunkinfo_free;

	log_debug("session[%d] create", ss->id);
	log_debug("recv file %d to %s", fileid, filepath);

	return mtk_work_schedule_withdata(ctx, mtk_session_start, 1*MTK_US_PER_MS, ss);
}

err_type mtk_voicebuff_recv(struct _mtk_context *ctx)
{
	mtk_session *ss;
	mtk_chunkinfo *ci;

	ci = mem_alloc_trace(sizeof(mtk_chunkinfo), "chunkinfo");
	if (!ci) {
		log_err("alloc chunkinfo error");
		return et_nomem;
	}
	memset(ci, 0x0, sizeof(mtk_chunkinfo));
	ci->fileid = 1;
	ci->index = 0;
	ci->type = MTK_CHUNKTYPE_BUFF;

	ss = mtk_session_clone(&chunkrecv);
	ss->id = mtk_session_id_get(ctx);
	ss->data = ci;
	ss->state = MTK_SESSION_REQ;
	ss->destory = mtk_chunkinfo_free;

	log_debug("session[%d] create", ss->id);

	return mtk_work_schedule_withdata(ctx, mtk_session_start, 1*MTK_US_PER_MS, ss);
}

err_type mtk_voicefile_send(struct _mtk_context *ctx, const char *filepath, int chunksize)
{
	err_type ret;
	mtk_session *ss;
	mtk_chunkinfo *ci;

	log_trace("file %s", filepath);

	ret = mtk_chunkinfo_generate_from_file(filepath, chunksize, &ci);
	if (ret != et_ok) {
		log_err("generate chunkinfo error:%d", ret);
		return ret;
	}

	ss = mtk_session_clone(&chunksend);
	ss->id = mtk_session_id_get(ctx);
	ss->data = ci;
	ss->state = MTK_SESSION_REQ;
	ss->destory = mtk_chunkinfo_free;

	log_debug("session[%d] create", ss->id);

	log_info("send file %s", filepath);

	return mtk_work_schedule_withdata(ctx, mtk_session_start, 1*MTK_US_PER_MS, ss);	
}