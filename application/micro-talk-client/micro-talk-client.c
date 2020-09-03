
#include "micro-talk.h"
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
	
	info.context = ctx;
	info.address = "172.16.79.229";
	info.port = 8972;
	info.channel = MTK_CHANNEL_NOTICE;

	ret = mtk_client_connect_via_info(&info);
	if (ret != et_ok) {
		log_err("connect error:%d", ret);
		ret = mtk_retry_work_schedule(ctx, nc_connect, 5);
		if (ret != et_ok) {
			log_err("schedule connect work error:%d", ret);
			/* send close signal */
			return et_exist;
		}
	}

	log_debug("connect finish");
	return et_ok;
}

err_type dc_connect(struct _awoke_minpq *wq)
{
	err_type ret;
	mtk_client_connect_info info;
	mtk_context *ctx = container_of(wq, struct _mtk_context, works_pq);

	info.context = ctx;
	info.address = "172.16.79.229";
	info.port = 8972;
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
	uint8_t idx = conn->msgindex + 1;
	conn->msgindex = idx % 255;
	return conn->msgindex;
}

err_type nc_build_heartbeat(struct _mtk_connect *conn)
{
	mtk_notice_msg msg;
	awoke_queue *ring = conn->ring;

	msg.data = nc_notice_index(conn);
	log_debug("msg:%d", msg);
	awoke_queue_enq(ring, &msg);
	return et_ok;
}

err_type nc_keepalive(struct _awoke_minpq *wq)
{
	int n;
	uint8_t d1=0x1, d2;

	mtk_context *ctx = container_of(wq, struct _mtk_context, works_pq);
	mtk_connect *conn = &ctx->connects[MTK_CHANNEL_NOTICE];

	log_debug("conn fd:%d", conn->fd);
	
	/* process */
	/*
	n = send(conn->fd, &d1, 1, 0);
	if (n < 0) {
		log_err("send error:%d", errno);
		return et_sock_send;
	}*/

	/*
	n = read(conn->fd, &d2, 1);
	if (n < 0) {
		log_err("read error:%d", errno);
		return et_sock_recv;
	}*/

	log_trace("read:%d data:0x%x", n, d2);

	#if 0
	if (d2 == d1) {
		log_info("heartbeat");
	} else if (d2 == 0x0) {
		log_info("notice");
		/* notice process */
		mtk_work_schedule(ctx, dc_connect, 1);
	} else {
		/* connect release */
		return et_sock_conn;
	}
	#endif
	
	mtk_work_schedule(ctx, nc_keepalive, 1000000*10);
	nc_build_heartbeat(conn);
	mtk_callback_on_writable(conn);
	return et_ok;
}

err_type service_callback(struct _mtk_connect *conn, mtk_callback_reasons reason,
	void *user, void *in, size_t len)
{
	err_type ret;

	switch (reason) {

	case MTK_CALLBACK_PROTOCOL_INIT:
		log_debug("MTK_CALLBACK_PROTOCOL_INIT");
		break;

	case MTK_CALLBACK_PROTOCOL_DESTROY:
		log_debug("MTK_CALLBACK_PROTOCOL_DESTROY");
		break;

	case MTK_CALLBACK_CONNECTION_REDIRECT:
		log_debug("MTK_CALLBACK_CONNECTION_REDIRECT");
		break;

	case MTK_CALLBACK_CONNECTION_ESTABLISHED:
		log_debug("MTK_CALLBACK_CONNECTION_ESTABLISHED");
		break;

	case MTK_CALLBACL_CONNECTION_RECEIVE:
		log_debug("MTK_CALLBACL_CONNECTION_RECEIVE");
		break;

	case MTK_CALLBACL_CONNECTION_WRITABLE:
		log_debug("MTK_CALLBACL_CONNECTION_WRITABLE");
		log_debug("channel:%d", conn->chntype);
		if (!awoke_queue_empty(conn->ring)) {
			mtk_notice_msg msg;
			awoke_queue_first(conn->ring, &msg);
			ret = mtk_connect_write(conn, &msg.data, 1);
			if (ret != et_ok) {
				log_err("write error:%d", ret);
				return ret;
			}
			awoke_queue_deq(conn->ring, &msg);

			if (!awoke_queue_empty(conn->ring)) {
				mtk_callback_as_writeable(conn);
			}
		}
		
		break;

	case MTK_CALLBACK_CONNECTION_CLOSED:
		log_debug("MTK_CALLBACK_CONNECTION_CLOSED");
		break;

	case MTK_CALLBACK_CONNECTION_ERROR:
		log_debug("MTK_CALLBACK_CONNECTION_ERROR");
		break;

	default:
		log_burst("ignore reason:%d", reason);
		break;
	}

	return et_ok;
}

int main(int argc, char **argv)
{
	err_type ret;
	mtk_context *ctx;

	ctx = mtk_context_create();
	if (!ctx) {log_err("mtk context create error"); return 0;}
	context = ctx;
	ctx->callback = service_callback;

	log_info("%s start", ctx->name);
	
	mtk_work_schedule(ctx, nc_keepalive, 1000000*10);
	mtk_work_schedule(ctx, nc_connect, 1);

	do {
		ret = mtk_service_run(ctx, 0);
		
	} while (!mtk_service_should_stop(ctx) &&
			 (ret == et_ok));
	
	log_notice("micro-talk service stop");

	return 0;
}
