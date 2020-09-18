
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
	info.address = ctx->server_address;
	info.port = ctx->nc_port;
	info.channel = MTK_CHANNEL_NOTICE;
	info.ka_timeout_us = 60*MTK_US_PER_SEC;
	info.callback = nc_service_callback;
	info.recv_timout_us = 10*MTK_US_PER_SEC;
	info.recv_timout_maxcnt = 5;

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

err_type mtk_dc_connect(struct _mtk_context *ctx, struct _mtk_work *work)
{
	err_type ret;
	mtk_client_connect_info info;

	memset(&info, 0x0, sizeof(info));
	info.context = ctx;
	info.port = ctx->dc_port;
	info.address = ctx->server_address;
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

err_type dc_response_recv(struct _mtk_connect *conn, void *in, size_t len)
{
	int dumplen;
	uint8_t *pos;
	uint8_t cmdtype = 0;
	uint32_t fileid = 0;
	err_type ret = et_ok;
	char method[8] = {'\0'};
	mtk_response_msg rsp;

	if (!conn || !in || !len)
		return et_param;

	//awoke_hexdump_trace((uint8_t *)in, len);
	rsp.recvlen = len;
	
	pos = (uint8_t *)in;
	pkg_pull_byte(rsp.mask, pos);
	rsp.ret = (rsp.mask & 0x80) >> 7;
	rsp.code = rsp.mask & 0x7f;
	pkg_pull_word(rsp.len, pos);
	rsp.len = htons(rsp.len);
	if (rsp.len) {
		rsp.data = pos;
		pkg_pull_dwrd(fileid, pos);
		fileid = htonl(fileid);
		pkg_pull_byte(cmdtype, pos);
		log_trace("fileid %d cmdtype %d", fileid, cmdtype);
	}
	pkg_pull_byte(rsp.checksum, pos);

	if (conn->last_method == MTK_METHOD_PUT) {
		sprintf(method, "PUT");
	} else if (conn->last_method == MTK_METHOD_GET) {
		sprintf(method, "GET");
	} else if (conn->last_method == MTK_METHOD_ALARM) {
		sprintf(method, "ALARM");
	}

	log_debug("%s Response(%d)", method, len);
	
	log_trace("");
	log_trace("%s Response:", method);
	log_trace("----------------");
	log_trace("ret:%d", rsp.ret);
	log_trace("code:%d", rsp.code);
	log_trace("length:%d", rsp.len);
	log_trace("checksum:%d", rsp.checksum);
	log_trace("----------------");
	log_trace("");

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

	dumplen = (len > 32) ? 32 : len;
	awoke_hexdump_trace(in, dumplen);

	if (conn->session) {
		return conn->session->rsp_process(conn, &rsp);
	} else {
		log_notice("no session to process");
		return et_ok;
	}

	if ((rsp.ret == 1)) {
		mtk_session_add(conn->context, &getcmd, NULL, NULL);
	} else if (rsp.len && fileid) {
		char filepath[32] = {'\0'};
		sprintf(filepath, "test-recv-%d", fileid);
		log_debug("fileid %d cmdtype %d", fileid, cmdtype);
		log_debug("will recv file to %s", filepath);
		mtk_voicefile_recv(conn->context, filepath, fileid);
	}

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
	msg.data = conn->msgindex;
	awoke_queue_enq(ring, &msg);
	log_info("Heartbeat(%d)", msg.data);
	return et_ok;
}

err_type nc_keepalive(struct _mtk_context *ctx, struct _mtk_work *work)
{
	mtk_connect *conn = (mtk_connect *)work->data;

	if (!conn->f_connected) {
		log_notice("nc disconnect");
		return et_fail;
	}
	
	nc_build_heartbeat(conn);
	mtk_callback_on_writable(conn);
	return et_ok;
}

err_type nc_notice_recv(struct _mtk_connect *conn, void *in, size_t len)
{
	mtk_notice_msg *msg;
	mtk_context *ctx = conn->context;
	
	msg = (mtk_notice_msg *)in;

	if (msg->data == 0x0) {
		log_info("Notice(%d), GET Command Request", msg->data);
		nc_notice_index(conn);
		mtk_work_schedule_withdata(ctx, nc_keepalive, conn->ka_timeout_us, conn);
		return mtk_session_add(ctx, &getcmd, NULL, NULL);
	} else {
		if (msg->data != conn->msgindex) {
			log_err("nc ack error, need:%d give:%d", conn->msgindex, msg->data);
			return et_fail;
		} else {
			log_info("Notice ack(%d)", msg->data);
			nc_notice_index(conn);
			mtk_work_schedule_withdata(ctx, nc_keepalive, conn->ka_timeout_us, conn);
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
		mtk_work_schedule(ctx, mtk_nc_connect, 10*MTK_US_PER_SEC);
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

	case MTK_CALLBACK_CONNECTION_RECEIVE_TIMEOUT:
		log_err("Heartbeat(%d) timeout", conn->msgindex);
		nc_build_heartbeat(conn);
		mtk_callback_on_writable(conn);
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
		log_debug("stop keepalive");
		mtk_work_delete(ctx, nc_keepalive);
		log_notice("retry connect after 30s");
		mtk_work_schedule(ctx, mtk_nc_connect, 30*MTK_US_PER_SEC);
		break;

	case MTK_CALLBACK_CONNECTION_ERROR:
		log_err("CONNECTION ERROR");
		log_debug("stop keepalive");
		mtk_work_delete(ctx, nc_keepalive);
		log_notice("retry connect after 30s");
		mtk_work_schedule(ctx, mtk_nc_connect, 30*MTK_US_PER_SEC);
		break;

	default:
		log_burst("ignore reason:%d", reason);
		break;
	}

	return et_ok;
}

