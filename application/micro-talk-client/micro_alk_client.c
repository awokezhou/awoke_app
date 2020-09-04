
#include "micro_talk.h"
#include <time.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>


static struct _mtk_context *context;

err_type nc_connect(struct _awoke_minpq *wq)
{
	err_type ret;
	mtk_client_connect_info info;
	mtk_context *ctx = container_of(wq, struct _mtk_context, works_pq);

	memset(&info, 0x0, sizeof(info));
	info.context = ctx;
	info.address = "172.16.79.229";
	info.port = 8972;
	info.channel = MTK_CHANNEL_NOTICE;
	info.ka_timeout_us = 30*MTK_US_PER_SEC;

	ret = mtk_client_connect_via_info(&info);
	if (ret != et_ok) {
		log_err("connect error:%d, will retry", ret);
		ret = mtk_work_schedule(ctx, nc_connect, 15*MTK_US_PER_SEC);
		//ret = mtk_retry_work_schedule(ctx, nc_connect, 5);
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

err_type dc_voice_send(struct _awoke_minpq *wq)
{
	FILE *fp;
	filerange range;
	
	fp = fopen(filename, mode);
	fseek(fp, range.start, SEEK_SET);
	fread(buff, range.size, 1, fp);

	build_voice_message():
	{
		/*
		 * range: start, end
		 * hex
	     */
		range.start = htons(range.start);
		range.end = htons(range.end);
	    pkg_push_word(range.start, pos);
		pkg_push_word(range.end, pos);
		pkg_push_bytes(buff, pos);
	}
}

err_type dc_connect(struct _awoke_minpq *wq)
{
	err_type ret;
	mtk_client_connect_info info;
	mtk_context *ctx = container_of(wq, struct _mtk_context, works_pq);

	memset(&info, 0x0, sizeof(info));
	info.context = ctx;
	info.port = 8970;
	info.address = "172.16.79.229";
	info.channel = MTK_CHANNEL_DATA;

	ret = mtk_client_connect_via_info(&info);
	if (ret != et_ok) {
		log_err("connect error:%d", ret);
		ret = mtk_retry_work_schedule(ctx, dc_connect, 5);
		if (ret != et_ok) {
			log_err("schedule connect work error:%d", ret);
			/* send close signal */
			return et_exist;
		}
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
	log_debug("build heartbeat(%d)", msg);
	return et_ok;
}

err_type dc_message_process(struct _mtk_connect *conn, void *in, size_t len)
{
	uint8_t code, checksum, *pos;
	uint16_t length;

	if (!conn || !in || len)
		return et_param;

	log_trace("GET Response(%d)", len);
	awoke_hexdump_trace((uint8_t *)in, len);

	pos = (uint8_t *)in;
	pkg_pull_byte(code, pos);
	pkg_pull_word(length, pos);
	length = htons(length);
	pkg_pull_byte(checksum, pos);

	log_debug("");
	log_debug("GET Response:");
	log_debug("----------------");
	log_debug("code:%d", code);
	log_debug("length:%d", length);
	log_debug("checksum:%d", checksum);
	log_debug("----------------");
	log_debug("");

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

err_type dc_build_get_request(struct _mtk_connect *conn)
{
	mtk_request_msg msg;

	/* 
	 * GET Request format: 
	 *   
	 * +----------------------------------------------+
	 * |	TAG  |	LEN  |	LBL  | TKN	|  DAT	| CHK |
	 * +----------------------------------------------+
	 * 
	 * TAG: 1 Byte for root type
	 * LEN: 2 Byte for packet length(0~65535)
	 * LBL: 1 Byte for sub type
	 * TKN: 8 Byte for token
	 * DAT: x Byte for private protocol data
	 * CHK: 1 Byte for check sum
	 */

	memset(&msg, 0x0, sizeof(msg));
	msg.tag = MTK_TAG_GET;
	msg.subtype = 0x0;
	msg.sublen = 0;
	msg.subdata = NULL;
	awoke_string_str2bcdv2(msg.token, "0123456789012347", 16);

	dc_request_msg_package(&msg);

	awoke_queue_enq(conn->ring, &msg);

	log_debug("build GET request");
	log_trace("GET Reqeust(%d):", msg.length);
	awoke_hexdump_trace(msg.payload, msg.length);

	return et_ok;
}

err_type nc_keepalive(struct _awoke_minpq *wq)
{
	mtk_context *ctx = container_of(wq, struct _mtk_context, works_pq);
	mtk_connect *conn = &ctx->connects[MTK_CHANNEL_NOTICE];
	mtk_work_schedule(ctx, nc_keepalive, conn->ka_timeout_us);
	nc_build_heartbeat(conn);
	mtk_callback_on_writable(conn);
	mtk_work_schedule(ctx, dc_connect, 1);
	return et_ok;
}

err_type service_callback(struct _mtk_connect *conn, mtk_callback_reasons reason,
	void *user, void *in, size_t len)
{
	err_type ret;
	mtk_context *ctx = conn->context;

	log_trace("channel:%d", conn->chntype);
	
	switch (reason) {

	case MTK_CALLBACK_PROTOCOL_INIT:
		log_trace("MTK_CALLBACK_PROTOCOL_INIT");
		if (conn->chntype == MTK_CHANNEL_NOTICE) {
			conn->ring = awoke_queue_create(sizeof(mtk_notice_msg), 8, AWOKE_QUEUE_F_RB);
		} else if (conn->chntype == MTK_CHANNEL_DATA) {
			conn->ring = awoke_queue_create(sizeof(mtk_request_msg), 8, AWOKE_QUEUE_F_RB);
		}
		break;

	case MTK_CALLBACK_PROTOCOL_DESTROY:
		log_trace("MTK_CALLBACK_PROTOCOL_DESTROY");
		break;

	case MTK_CALLBACK_CONNECTION_REDIRECT:
		log_trace("MTK_CALLBACK_CONNECTION_REDIRECT");
		break;

	case MTK_CALLBACK_CONNECTION_ESTABLISHED:
		log_trace("MTK_CALLBACK_CONNECTION_ESTABLISHED");
		if (conn->chntype == MTK_CHANNEL_NOTICE) {
			log_debug("keepalive start(%ds)", conn->ka_timeout_us/MTK_US_PER_SEC);
			mtk_work_schedule(ctx, nc_keepalive, conn->ka_timeout_us);
		} else if (conn->chntype == MTK_CHANNEL_DATA) {
			dc_build_get_request(conn);
			mtk_callback_on_writable(conn);
		}
		break;

	case MTK_CALLBACL_CONNECTION_RECEIVE:
		log_trace("MTK_CALLBACL_CONNECTION_RECEIVE");
		log_trace("recv length:%d", len);
		if (conn->chntype == MTK_CHANNEL_NOTICE) {
			mtk_notice_msg *msg = (mtk_notice_msg *)in;
			if (msg->data == 0x0) {
				log_info("recv notice msg(%d)", msg->data);
			} else {
				if (msg->data != conn->msgindex) {
					log_err("nc ack error, need:%d give:%d", conn->msgindex, msg->data);
				} else {
					log_info("recv notice ack(%d)", msg->data);
				}
			}
		} else if (conn->chntype == MTK_CHANNEL_DATA) {
			log_info("recv Response pkt");
			dc_message_process(conn, in, len);
		}
		break;

	case MTK_CALLBACL_CONNECTION_WRITABLE:

		log_trace("MTK_CALLBACL_CONNECTION_WRITABLE");

		if (!awoke_queue_empty(conn->ring)) {

			log_trace("ring remain:%d", awoke_queue_size(conn->ring));
		
			if (conn->chntype == MTK_CHANNEL_NOTICE) {
				log_info("send Heartbeat pkt");
				mtk_notice_msg msg;
				awoke_queue_first(conn->ring, &msg);
				ret = mtk_connect_write(conn, &msg.data, 1);
				if (ret != et_ok) {
					log_err("write error:%d", ret);
					return ret;
				}
				awoke_queue_deq(conn->ring, &msg);				
			} else if (conn->chntype == MTK_CHANNEL_DATA) {
				log_info("send GET Request pkt");
				mtk_request_msg msg;
				awoke_queue_first(conn->ring, &msg);
				ret = mtk_connect_write(conn, &msg.payload, msg.length);
				if (ret != et_ok) {
					log_err("write error:%d", ret);
					return ret;
				}
				mem_free(msg.payload);
				awoke_queue_deq(conn->ring, &msg);
			}

			if (!awoke_queue_empty(conn->ring)) {
				log_trace("remain msg");
				mtk_callback_on_writable(conn);
			}
		}
		
		break;

	case MTK_CALLBACK_CONNECTION_CLOSED:
		log_trace("MTK_CALLBACK_CONNECTION_CLOSED");
		break;

	case MTK_CALLBACK_CONNECTION_ERROR:
		log_trace("MTK_CALLBACK_CONNECTION_ERROR");
		break;

	default:
		log_trace("ignore reason:%d", reason);
		break;
	}

	return et_ok;
}

int main(int argc, char **argv)
{
	err_type ret;
	mtk_context *ctx;

	awoke_log_init(LOG_INFO, LOG_DBG);

	ctx = mtk_context_create();
	if (!ctx) {log_err("mtk context create error"); return 0;}
	context = ctx;
	ctx->callback = service_callback;

	log_info("%s start", ctx->name);
	
	mtk_work_schedule(ctx, nc_connect, 0);
	mtk_work_schedule(ctx, dc_connect, 1);

	/* service loop run */
	do {

		ret = mtk_service_run(ctx, 0);
		
	} while (!mtk_service_should_stop(ctx) &&
			 (ret == et_ok));
	
	log_notice("%s stop", ctx->name);

	return 0;
}
