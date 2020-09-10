
#include "micro_talk.h"
#include "mtk_client.h"
#include "mtk_session.h"
#include "mtk_protocol.h"
#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <sys/socket.h>
#include "awoke_string.h"



err_type nc_service_callback(struct _mtk_connect *conn, mtk_callback_reasons reason,
	void *user, void *in, size_t len);
err_type dc_service_callback(struct _mtk_connect *conn, mtk_callback_reasons reason,
	void *user, void *in, size_t len);



err_type mtk_nc_connect(struct _mtk_context *ctx, struct _mtk_work *work)
{
	err_type ret;
	mtk_client_connect_info info;

	memset(&info, 0x0, sizeof(info));
	info.context = ctx;
	info.address = "172.16.79.229";
	info.port = 8972;
	info.channel = MTK_CHANNEL_NOTICE;
	info.ka_timeout_us = 30*MTK_US_PER_SEC;
	info.callback = nc_service_callback;

	ret = mtk_client_connect_via_info(&info);
	if (ret != et_ok) {
		log_err("connect error:%d, will retry", ret);
		ret = mtk_work_schedule(ctx, mtk_nc_connect, 15*MTK_US_PER_SEC);
		//ret = mtk_retry_work_schedule(ctx, mtk_nc_connect, 5);
		if (ret != et_ok) {
			log_err("schedule connect work error:%d", ret);
			/* send close signal */
			return et_exist;
		} else {
			return et_ok;
		}
	}

	return et_ok;
}

#if 0
err_type dc_build_file_chunk(uint8_t *offset, char *filepath, int start, int size)
{
	int n;
	FILE *fp;
	uint8_t *buf;

	buf = mem_alloc_trace(size, "read");
	if (!buf) {
		log_err("alloc file buffer error");
		return et_nomem;
	}

	fp = fopen(filepath, "rb");
	if (!fp) {
		log_err("file open error");
		return et_file_open;
	}

	fseek(fp, start, SEEK_SET);

	n = fread(buf, 1, size, fp);
	if (n != size) {
		if (n < 0) {
			log_err("file read error");
			fclose(fp);
			return et_file_read;
		}
	}

	fclose(fp);

	memcpy(offset, buf, size);
	mem_free(buf);

	return et_ok;
}

err_type dc_message_checksum(struct _mtk_request_msg *m, int len)
{
	int i;

	m->checksum = 0;
	
	for (i=0; i<len; i++) {
		m->checksum += m->payload[i];
	}

	return et_ok;
}
#endif
err_type dc_build_message(uint8_t *offset, uint8_t method, uint16_t sublen, uint8_t lable)
{
	uint16_t length;
	uint8_t *pos = offset;
	uint8_t token[8];
	
	awoke_string_str2bcdv2(token, "0123456789012347", 16);
	
	pkg_push_byte(method, pos);
	length = htons(sublen);
	pkg_push_word(length, pos);
	pkg_push_byte(lable, pos);
	pkg_push_bytes(token, pos, 8);

	return et_ok;
}

#if 0
err_type filechunk_write(const char *filepath, char *buff, int size)
{
	int n;
	FILE *fp;

	fp = fopen(filepath, "ab+");
	if (!fp) {
		log_err("open file error");
		return et_file_open;
	}

	n = fwrite(buff, size, 1, fp);
	if (n <= 0) {
		log_err("write error");
		fclose(fp);
		return et_file_send;
	}

	fclose(fp);

	return et_ok;
}
#endif

#if 0
err_type dc_filechunk_save(struct filechunk *chunkinfo)
{
	int prior;
	err_type ret;
	struct filebuff fb;
	
	while (!awoke_minpq_empty(chunkinfo->fq)) {

		awoke_minpq_delmin(chunkinfo->fq, &fb, &prior);
		
		ret = filechunk_write("test", fb.data, fb.len);
		if (ret != et_ok) {
			log_err("write file error:%d", ret);
			return ret;
		}

		log_trace("write %d", fb.len);
	}

	return et_ok;
}
#endif

#if 0
err_type dc_response_get_recv(struct _mtk_connect *c, struct _mtk_response_msg *rsp)
{
	mtk_context *ctx = c->context;
	struct filebuff fb;
	uint8_t code, *pos=rsp->data;
	uint16_t chunk_number;
	uint32_t filesize;
	uint16_t crc;

	pkg_pull_byte(code, pos);
	pkg_pull_word(chunk_number, pos);
	chunk_number = htons(chunk_number);
	pkg_pull_dwrd(filesize, pos);
	filesize = htonl(filesize);
	pkg_pull_word(crc, pos);
	crc = htons(crc);

	log_debug("");
	log_debug("filechunk:");
	log_debug("----------------");
	log_debug("code:%d", code);
	log_debug("chunk number:%d", chunk_number);
	log_debug("filesize:%d", filesize);
	log_debug("CRC:0x%x", crc);
	log_debug("----------------");
	log_debug("");

	if (code == 0x0) {
		log_debug("recv success");
		log_debug("index:%d size:%d", myfilechunk.index, rsp->len-9);

		if (!myfilechunk.fq) {
			log_debug("create file queue");
			myfilechunk.fq = awoke_minpq_create(sizeof(fb), chunk_number, NULL, 0x0);
			if (!myfilechunk.fq) {
				log_err("create file queue error");
				return et_nomem;
			}

			myfilechunk.number = chunk_number;
			myfilechunk.filesize = filesize;
			myfilechunk.crc = crc;
		}

		fb.data = mem_alloc_trace(rsp->len-9, "filebuff");
		memcpy(fb.data, rsp->data+9, rsp->len-9);
		fb.len = rsp->len-9;
		awoke_minpq_insert(myfilechunk.fq, &fb, myfilechunk.index);

		if (myfilechunk.index >= (myfilechunk.number-1)) {
			log_info("file recv finish");
			log_debug("file write to ...");
			dc_filechunk_save(&myfilechunk);
		} else {
			mtk_work_schedule_withdata(ctx, dc_file_recv, 1, c);
		}

		myfilechunk.index++;
	}

	return et_ok;
}
#endif

#if 0
err_type dc_response_put_recv(struct _mtk_connect *c, struct _mtk_response_msg *rsp)
{
	mtk_context *ctx = c->context;
	uint8_t code=0, *pos=rsp->data;
	uint32_t fileid;

	if (!c || !rsp || !rsp->len || !rsp->data)
		return et_param;

	pkg_pull_byte(code, pos);
	pkg_pull_dwrd(fileid, pos);
	fileid = htonl(fileid);

	log_debug("code:%d", code);
	log_debug("fileid:%d", fileid);

	if (code == 0x00) {
		log_info("chunk transmit success");
		myfilechunk.index++;
		mtk_work_schedule(ctx, dc_file_send, 1*MTK_US_PER_MS);
	} else if (code == 0x01) {
		log_info("allchunk transmit success");
		myfilechunk.start = FALSE;
	} else if (code == 0x03) {
		log_err("file checksum error");
		mtk_work_schedule(ctx, dc_file_send, 1*MTK_US_PER_MS);
	} else if (code == 0x04) {
		log_err("chunk transmit fail");
		mtk_work_schedule(ctx, dc_file_send, 1*MTK_US_PER_MS);
	}

	return et_ok;
}
#endif
#if 0
err_type dc_filechunk_send(struct _mtk_context *ctx, struct _mtk_work *work)
{
	mtk_connect *conn = &ctx->connects[MTK_CHANNEL_DATA];
	struct filechunk *ci = work->data;

	log_debug("dc_filechunk_send");

	int i, len;
	uint8_t *buff, *pos, *offset, checksum=0;
	uint16_t chunk_nr, chunk_index, crc;
	uint32_t filesize, hash, start;
	
	mtk_request_msg msg;

	if (ci->index > (ci->number-1)) {
		log_info("filechunk finish");
		return et_ok;
	}

	buff = mem_alloc_trace(13+12+ci->chunksize, "filechunk");
	if (!buff) {
		log_err("alloc buffer error");
		return et_nomem;
	}

	pos = buff + 12;

	/* chunk number */
	chunk_nr = htons(ci->number);
	pkg_push_word(chunk_nr, pos);

	/* chunk index */
	chunk_index = htons(ci->index);
	pkg_push_word(chunk_index, pos);

	/* file size */
	filesize = htonl(ci->filesize);
	pkg_push_dwrd(filesize, pos);

	/* crc */
	crc = htons(ci->crc);
	pkg_push_word(crc, pos);

	/* hash */
	hash = htons(ci->hash);
	pkg_push_word(hash, pos);

	start = (ci->index == 0) ? 0 : ci->index*ci->chunksize;
	len = (ci->index == (ci->number-1)) ? (ci->filesize-start) : ci->chunksize;
	dc_build_file_chunk(pos, ci->filepath, start, len);
	log_debug("index:%d [%llu:%llu] size:%d", ci->index, start, 
		start+len-1, len);

	pos += len;
	offset = buff;
	dc_build_message(offset, MTK_METHOD_PUT, 12+len, MTK_LBL_COMMAND);
	
	len = pos-buff;
	for (i=0; i<len; i++) {
		checksum += buff[i];
	}
	pkg_push_byte(checksum, pos);
	
	memset(&msg, 0x0, sizeof(msg));
	msg.payload = buff;
	msg.length = pos-buff;
	log_debug("msg.length:%d", msg.length);

	//awoke_hexdump_trace(msg.payload, msg.length);

	awoke_queue_enq(conn->ring, &msg);

	mtk_callback_on_writable(conn);

	return et_ok;
}
#endif

#if 0
static int getfilesize(const char *filepath)
{
	struct stat statbuff;

	if (stat(filepath, &statbuff) < 0) {
		return -1;
	} else {
		return statbuff.st_size;
	}
}

static uint16_t getfilecrc(const char *filepath, int filesize)
{
	int n;
	FILE *fp;
	uint8_t *buf;
	uint16_t crc;

	if (!filepath || !filesize)
		return 0;

	buf = mem_alloc_trace(filesize, "read");
	if (!buf) {
		log_err("alloc file buffer error");
		return 0;
	}

	fp = fopen(filepath, "rb");
	if (!fp) {
		log_err("open error");
		return 0;
	}

	n = fread(buf, 1, filesize, fp);
	if (n != filesize) {
		log_err("file read error:%d", ferror(fp));
		fclose(fp);
		return 0;
	}

	fclose(fp);

	crc = awoke_crc16(buf, filesize);

	mem_free(buf);

	return crc;
}
#endif
#if 0
err_type file_chunk_generate(const char *filepath, int chunksize, struct filechunk *c)
{
	int filesize;
		
	if (!filepath || !c)
		return et_param;

	memset(c, 0x0, sizeof(struct filechunk));

	filesize = getfilesize(filepath);
	if (filesize <= 0) {
		log_err("get file size error");
		return et_file_open;
	}

	c->crc = getfilecrc(filepath, filesize);
	if (!c->crc) {
		log_err("get file CRC error");
		return et_param;
	}
	
	c->hash = 6;
	c->index = 0;
	if (filesize <= chunksize)
		c->number = 1;
	else
		c->number = (filesize % chunksize) ? (filesize/chunksize+1) : filesize/chunksize;
	c->filesize = filesize;
	c->filepath = awoke_string_dup(filepath);
	c->chunksize = filesize <= chunksize ? filesize : chunksize;

	return et_ok;
}
#endif
/*
err_type dc_chunkmsg_send(struct _mtk_context *ctx, struct _mtk_work *work)
{
	mtk_chunkmsg *m = (mtk_chunkmsg)work->data;
	mtk_connect *conn = &ctx->connects[MTK_CHANNEL_DATA];

	if (!conn->f_connected) {

		if (!conn->f_connecting)
			mtk_work_schedule(ctx, mtk_dc_connect, 1);
		
		return mtk_work_schedule(ctx, dc_chunkmsg_send, 2);

	} else {

		return mtk_work_schedule(ctx, chunkmsg_send, 2, m);
	
	}
}*/

#if 0
err_type dc_filechunk_recv2(struct _mtk_connect *conn)
{
	struct filechunk *ci = conn->session->data;

	int len;
	uint8_t *buff, *pos;
	uint16_t index;
	uint32_t fileid;
	mtk_request_msg msg;
	
	buff = mem_alloc_trace(13+6, "GET FILE");
	if (!buff) {
		log_err("alloc GET Request error");
		return et_nomem;
	}

	pos = buff + 12;

	fileid = htonl(ci->fileid);
	pkg_push_dwrd(fileid, pos);
	index = htons(ci->index);
	pkg_push_word(index, pos);

	dc_build_message(buff, MTK_METHOD_GET, 6, MTK_LBL_COMMAND);
	conn->last_method = MTK_METHOD_GET;
	len = pos-buff;
	memset(&msg, 0x0, sizeof(msg));
	msg.payload = buff;
	dc_message_checksum(&msg, len);
	pkg_push_byte(msg.checksum, pos);
	msg.length = pos-buff;

	log_debug("msg.length:%d", msg.length);
	log_debug("index:%d", ci->index);

	awoke_hexdump_trace(msg.payload, msg.length);

	awoke_queue_enq(conn->ring, &msg);

	mtk_callback_on_writable(conn);

	return et_ok;
}
#endif 
#if 0
err_type dc_filechunk_recv(struct _mtk_context *ctx, struct _mtk_work *work)
{
	mtk_connect *conn = &ctx->connects[MTK_CHANNEL_DATA];
	struct filechunk *ci = work->data;

	int len;
	uint8_t *buff, *pos;
	uint16_t index;
	uint32_t fileid;
	mtk_request_msg msg;
	
	buff = mem_alloc_trace(13+6, "GET FILE");
	if (!buff) {
		log_err("alloc GET Request error");
		return et_nomem;
	}

	pos = buff + 12;

	fileid = htonl(ci->fileid);
	pkg_push_dwrd(fileid, pos);
	index = htons(ci->index);
	pkg_push_word(index, pos);

	dc_build_message(buff, MTK_METHOD_GET, 6, MTK_LBL_COMMAND);
	conn->last_method = MTK_METHOD_GET;
	len = pos-buff;
	memset(&msg, 0x0, sizeof(msg));
	msg.payload = buff;
	dc_message_checksum(&msg, len);
	pkg_push_byte(msg.checksum, pos);
	msg.length = pos-buff;

	log_debug("msg.length:%d", msg.length);
	log_debug("index:%d", ci->index);

	awoke_hexdump_trace(msg.payload, msg.length);

	awoke_queue_enq(conn->ring, &msg);

	mtk_callback_on_writable(conn);

	return et_ok;
}

err_type dc_file_recv(struct _mtk_context *ctx, struct _mtk_work *work)
{
	mtk_connect *conn = &ctx->connects[MTK_CHANNEL_DATA];

	if (!conn->f_connected) {
		if (!conn->f_connecting) {
			mtk_work_schedule(ctx, mtk_dc_connect, 1);
		}
		mtk_work_schedule(ctx, dc_file_recv, 1*MTK_US_PER_MS);
	} else if (conn->f_connected) {

		if (!myfilechunk.start) {
			myfilechunk.fileid = 1;
			myfilechunk.index = 0;
			myfilechunk.start = TRUE;
			myfilechunk.fq = NULL;
		}
		
		mtk_work_schedule_withdata(ctx, dc_filechunk_recv, 1*MTK_US_PER_MS, &myfilechunk);
	}

	return et_ok;
}

err_type dc_file_send(struct _mtk_context *ctx, struct _mtk_work *work)
{
	char *filepath = "linux-c-api-ref.pdf";
	mtk_connect *conn = &ctx->connects[MTK_CHANNEL_DATA];
	
	if (!conn->f_connected) {
		if (!conn->f_connecting) {
			mtk_work_schedule(ctx, mtk_dc_connect, 1);
		}
		mtk_work_schedule(ctx, dc_file_send, 1*MTK_US_PER_MS);
	} else if (conn->f_connected) {

		if (!myfilechunk.start) {
			file_chunk_generate(filepath, 1024*8, &myfilechunk);
			log_debug("");
			log_debug("filechunk start");
			log_debug("----------------");
			log_debug("filepath:%s", filepath);
			log_debug("filesize:%d(bytes)", myfilechunk.filesize);
			log_debug("chunksize:%d", myfilechunk.chunksize);
			log_debug("number:%d", myfilechunk.number);
			log_debug("CRC:0x%x", myfilechunk.crc);
			log_debug("----------------");
			log_debug("");
			myfilechunk.start = TRUE;
			mtk_work_schedule(ctx, dc_file_send, 1*MTK_US_PER_MS);
		} else {
			mtk_work_schedule_withdata(ctx, dc_filechunk_send, 1*MTK_US_PER_MS, &myfilechunk);
		}
	}

	return et_ok;
}
#endif
err_type mtk_dc_connect(struct _mtk_context *ctx, struct _mtk_work *work)
{
	err_type ret;
	mtk_client_connect_info info;

	memset(&info, 0x0, sizeof(info));
	info.context = ctx;
	info.port = 8970;
	info.address = "172.16.79.229";
	info.channel = MTK_CHANNEL_DATA;
	info.callback = dc_service_callback;

	ret = mtk_client_connect_via_info(&info);
	if (ret != et_ok) {
		log_err("connect error:%d", ret);
		ret = mtk_retry_work_schedule(ctx, mtk_dc_connect, 5);
		if (ret != et_ok) {
			log_err("schedule connect work error:%d", ret);
			/* send close signal */
			return et_exist;
		}
	}

	return et_ok;
}

err_type dc_request_msg_package(struct _mtk_request_msg *m)
{
	int i, length;
	uint8_t *pos;
	
	if (!m) {
		return et_param;
	}

	if (m->payload) {
		log_err("payload exist");
		return et_param;
	}

	if (m->checksum) {
		log_err("checksum exist");
		return et_param;
	}

	m->payload = mem_alloc_trace(13+m->sublen, "request pkt");
	if (!m->payload) {
		log_err("alloc error");
		return et_nomem;
	}

	pos = m->payload;
	
	pkg_push_byte(m->tag, pos);
	m->sublen = htons(m->sublen);
	pkg_push_word(m->sublen, pos);
	pkg_push_byte(m->subtype, pos);
	pkg_push_bytes(m->token, pos, 8);

	if (m->subdata) {
		pkg_push_bytes(m->subdata, pos, m->sublen);
	}

	length = pos - m->payload;
	for (i=0; i<length; i++) {
		m->checksum += m->payload[i];
	}

	pkg_push_byte(m->checksum, pos);

	m->length = pos - m->payload;

	return et_ok;
}

err_type dc_normal_get(struct _mtk_context *ctx, struct _mtk_work *work)
{
	mtk_request_msg msg;
	mtk_connect *conn = (mtk_connect *)work->data;

	if (!conn->f_connected) {
		log_debug("not connected");
		if (!conn->f_connecting) {
			mtk_work_schedule(ctx, mtk_dc_connect, 1);
		}
		mtk_work_schedule_withdata(ctx, dc_normal_get, 1*MTK_US_PER_SEC, conn);
	} else if (conn->f_connected) {
		msg.payload = mem_alloc_trace(13, "GET Request");
		if (!msg.payload) {
			log_err("alloc GET Request error");
			return et_nomem;
		}

		mtk_pkg_request_header(&msg, MTK_METHOD_GET, 0, MTK_LBL_COMMAND);
		
		awoke_hexdump_trace(msg.payload, msg.length);
		
		awoke_queue_enq(conn->ring, &msg);
		
		mtk_callback_on_writable(conn);
		log_info("GET Request");
	}

	return et_ok;
}

err_type dc_response_recv(struct _mtk_connect *conn, void *in, size_t len)
{
	uint8_t *pos;
	err_type ret = et_ok;
	char method[8] = {'\0'};
	mtk_response_msg rsp;

	if (!conn || !in || !len)
		return et_param;

	//awoke_hexdump_trace((uint8_t *)in, len);
	rsp.recvlen = len;
	
	pos = (uint8_t *)in;
	pkg_pull_byte(rsp.code, pos);
	pkg_pull_word(rsp.len, pos);
	rsp.len = htons(rsp.len);
	if (rsp.len) {
		rsp.data = pos;
	}
	pos += rsp.len;
	pkg_pull_byte(rsp.checksum, pos);

	if (conn->last_method == MTK_METHOD_PUT) {
		sprintf(method, "PUT");
	} else if (conn->last_method == MTK_METHOD_GET) {
		sprintf(method, "GET");
	} else if (conn->last_method == MTK_METHOD_ALARM) {
		sprintf(method, "ALARM");
	}
	
	log_debug("");
	log_debug("%s Response:", method);
	log_debug("----------------");
	log_debug("code:%d", rsp.code);
	log_debug("length:%d", rsp.len);
	log_debug("checksum:%d", rsp.checksum);
	log_debug("----------------");
	log_debug("");

	/*
	 * Response code
	 *     0x0: success
	 *     0x1: checksum error
	 *	   0x2: can't find label
	 */

	if (rsp.code == 0x0) {
		log_debug("response success");
	} else if (rsp.code == 0x1) {
		log_err("checksum error");
		return et_fail;
	} else if (rsp.code == 0x2) {
		log_err("can't find label");
		return et_fail;
	}

	if (!rsp.len) {
		log_trace("no more data");
		return et_ok;
	}

	if (conn->session->rsp_process)
		return conn->session->rsp_process(conn, &rsp);

	return ret;
}

err_type dc_service_callback(struct _mtk_connect *conn, mtk_callback_reasons reason,
	void *user, void *in, size_t len)
{
	err_type ret;
	mtk_request_msg msg;

	switch (reason) {

	case MTK_CALLBACK_PROTOCOL_INIT:
		conn->ring = awoke_queue_create(sizeof(mtk_request_msg), 8, AWOKE_QUEUE_F_RB);
		if (!conn->ring) {
			log_err("create ring error");
			return et_nomem;
		}
		log_trace("create ring(%d)", conn->ring->node_nr);
		break;

	case MTK_CALLBACK_PROTOCOL_DESTROY:
		break;

	case MTK_CALLBACK_CONNECTION_ESTABLISHED:
		if (conn->session) {
			conn->session->req_process(conn);
		}
		break;

	case MTK_CALLBACL_CONNECTION_RECEIVE:
		dc_response_recv(conn, in, len);
		break;

	case MTK_CALLBACL_CONNECTION_WRITABLE:
		
		if (!awoke_queue_empty(conn->ring)) {
			
			awoke_queue_deq(conn->ring, &msg);
			
			ret = mtk_connect_write(conn, msg.payload, msg.length);
			if (ret != et_ok) {
				log_err("write msg error:%d", ret);
				awoke_queue_enq(conn->ring, &msg);
				mtk_callback_on_writable(conn);
				return ret;
			}

			mem_free(msg.payload);

			if (!awoke_queue_empty(conn->ring)) {
				log_trace("remain msg");
				mtk_callback_on_writable(conn);
			}
		}
		
		break;

	case MTK_CALLBACK_CONNECTION_CLOSED:
		log_notice("CONNECTION CLOSED");
		break;

	case MTK_CALLBACK_CONNECTION_ERROR:
		log_notice("CONNECTION ERROR");
		break;

	default:
		log_burst("ignore reason:%d", reason);
		break;
	}

	return et_ok;
}

uint8_t nc_notice_index(struct _mtk_connect *conn)
{
	conn->msgindex = (conn->msgindex % 255) + 1;
	return conn->msgindex;
}

err_type nc_build_heartbeat(struct _mtk_connect *conn)
{
	mtk_notice_msg msg;
	awoke_queue *ring = conn->ring;
	msg.data = nc_notice_index(conn);
	awoke_queue_enq(ring, &msg);
	log_info("Heartbeat(%d)", msg.data);
	return et_ok;
}

err_type nc_keepalive(struct _mtk_context *ctx, struct _mtk_work *work)
{
	mtk_connect *conn = (mtk_connect *)work->data;
	nc_build_heartbeat(conn);
	mtk_callback_on_writable(conn);
	mtk_work_schedule_withdata(ctx, nc_keepalive, conn->ka_timeout_us, conn);
	return et_ok;
}

err_type nc_notice_recv(struct _mtk_connect *conn, void *in, size_t len)
{
	mtk_notice_msg *msg;
	mtk_context *ctx = conn->context;
	
	msg = (mtk_notice_msg *)in;

	if (msg->data == 0x0) {
		log_info("Notice(%d)", msg->data);
		return mtk_work_schedule_withdata(conn->context, dc_normal_get, 1, 
			&ctx->connects[MTK_CHANNEL_DATA]);
	} else {
		if (msg->data != conn->msgindex) {
			log_err("nc ack error, need:%d give:%d", conn->msgindex, msg->data);
			return et_fail;
		} else {
			log_info("Notice ack(%d)", msg->data);
		}
	}

	return et_ok;
}

err_type nc_service_callback(struct _mtk_connect *conn, mtk_callback_reasons reason,
	void *user, void *in, size_t len)
{
	err_type ret;
	mtk_notice_msg msg;
	mtk_context *ctx = conn->context;
	
	switch (reason) {

	case MTK_CALLBACK_PROTOCOL_INIT:
		conn->ring = awoke_queue_create(sizeof(mtk_notice_msg), 8, AWOKE_QUEUE_F_RB);
		if (!conn->ring) {
			log_err("create ring error");
			return et_nomem;
		}
		log_trace("create ring(%d)", conn->ring->node_nr);
		break;

	case MTK_CALLBACK_PROTOCOL_DESTROY:
		log_err("protocol destory");
		log_debug("reconnect after %d(s)", 60*MTK_US_PER_SEC);
		mtk_work_schedule(ctx, mtk_nc_connect, 60*MTK_US_PER_SEC);
		break;

	case MTK_CALLBACK_CONNECTION_REDIRECT:
		break;

	case MTK_CALLBACK_CONNECTION_ESTABLISHED:
		log_debug("keepalive start(%ds)", conn->ka_timeout_us/MTK_US_PER_SEC);
		mtk_work_schedule_withdata(ctx, nc_keepalive, conn->ka_timeout_us, conn);
		break;

	case MTK_CALLBACL_CONNECTION_RECEIVE:
		ret = nc_notice_recv(conn, in, len);
		if (ret != et_ok) {
			log_err("receive notice error:%d", ret);
			return ret;
		}
		break;

	case MTK_CALLBACL_CONNECTION_WRITABLE:

		if (!awoke_queue_empty(conn->ring)) {
		
			awoke_queue_deq(conn->ring, &msg);
			ret = mtk_connect_write(conn, &msg.data, 1);
			if (ret != et_ok) {
				log_err("write error:%d", ret);
				awoke_queue_enq(conn->ring, &msg);
				mtk_callback_on_writable(conn);
				return ret;
			}

			if (!awoke_queue_empty(conn->ring)) {
				log_trace("remain msg");
				mtk_callback_on_writable(conn);
			}
		}
		
		break;

	case MTK_CALLBACK_CONNECTION_CLOSED:
		log_err("CONNECTION CLOSED");
		break;

	case MTK_CALLBACK_CONNECTION_ERROR:
		log_err("CONNECTION ERROR");
		break;

	default:
		log_burst("ignore reason:%d", reason);
		break;
	}

	return et_ok;
}


#if 0
int main(int argc, char **argv)
{
	err_type ret;
	mtk_context *ctx;

	awoke_log_init(LOG_DBG, LOG_DBG);

	ctx = mtk_context_create();
	if (!ctx) {log_err("mtk context create error"); return 0;}
	context = ctx;

	log_info("%s start", ctx->name);
	
	//mtk_work_schedule(ctx, mtk_nc_connect, 0);
	//mtk_work_schedule_withdata(ctx, dc_normal_get, 1, &ctx->connects[MTK_CHANNEL_DATA]);
	//mtk_work_schedule(ctx, dc_file_send, 1*MTK_US_PER_SEC);
	//mtk_work_schedule(ctx, dc_file_recv, 1*MTK_US_PER_SEC);
	
	
	/* service loop run */
	do {

		ret = mtk_service_run(ctx, 0);
		
	} while (!mtk_service_should_stop(ctx) &&
			 (ret == et_ok));
	
	log_notice("%s stop", ctx->name);

	return 0;
}
#endif
