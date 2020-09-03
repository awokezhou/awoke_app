
#include "micro-talk.h"
#include "awoke_log.h"
#include <netdb.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "md5.h"

err_type mtk_handle_write_event(struct _mtk_context *ctx, struct _mtk_connect *conn);
err_type light_link_read(struct _mtk_context *ctx, struct _mtk_connect *conn);
err_type light_link_write(struct _mtk_context *ctx, struct _mtk_connect *conn);

static mtk_protocol_handle proto_light_link = {
	.name = "LightLink",
	.handle_read = light_link_read,
	.handle_write = light_link_write,
};

err_type light_link_read(struct _mtk_context *ctx, struct _mtk_connect *conn)
{
	if (conn->event.mask & EVENT_WRITE) {
		return mtk_handle_write_event(ctx, conn);
	}

	/* read data */

	return et_ok;
}

err_type light_link_write(struct _mtk_context *ctx, struct _mtk_connect *conn)
{
	return et_ok;
}

uint64_t mtk_now_usec()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return ((unsigned long long)tv.tv_sec * 1000000LL) + tv.tv_usec;
}

void mtk_context_release(struct _mtk_context *ctx)
{
	if (ctx->evloop) {
		awoke_event_loop_clean(&ctx->evloop);
	}
	
	mem_free_trace(ctx, "mtk context");
}

static bool work_us_comparator(void *d1, void *d2)
{
	mtk_work *w1=(mtk_work *)d1, *w2=(mtk_work *)d2;

	return (w1->us > w2->us);
}

void mtk_event_change_mask(struct _awoke_event_loop *evl, struct _awoke_event *e, 
	uint32_t and, uint32_t or, void *data)
{
	awoke_event_del(evl, e);
	e->mask = (e->mask & ~and) | or;
	awoke_event_add(evl, e->fd, e->type, e->mask, data);
}

err_type mtk_callback_on_writable(struct _mtk_connect *conn)
{
	mtk_event_change_mask(conn->context->evloop, &conn->event, 0x0, EVENT_WRITE, conn);
	return et_ok;
}

err_type mtk_callback_as_writeable(struct _mtk_connect *conn)
{
	return conn->context->callback(conn, MTK_CALLBACL_CONNECTION_WRITABLE, conn->user, NULL, 0);
}

err_type mtk_handle_write_event(struct _mtk_context *ctx, struct _mtk_connect *conn)
{
	int n;
	err_type ret;
	
	ret = conn->protocol->handle_write(ctx, conn);
	if (ret != et_ok) {
		log_err("protocol handle write error:%d", ret);
		return ret;
	}

	if (!conn->leave_wirtable_active) {
		mtk_event_change_mask(ctx->evloop, &conn->event, EVENT_WRITE, 0x0, conn);
	}

	ret = mtk_callback_as_writeable(conn);
	return ret;
}

struct _mtk_context *mtk_context_create(void)
{
	err_type ret;
	
	mtk_context *ctx = mem_alloc_trace(sizeof(mtk_context), "mtk context");
	if (!ctx) {
		log_err("mtk context create error");
		return NULL;
	}

	ctx->service_need_stop = FALSE;

	ctx->name = awoke_string_dup("MTK Client");

	ret = awoke_minpq_init(&ctx->works_pq, sizeof(mtk_work), 8, 
		work_us_comparator, 0x0);
	if (ret != et_ok) {
		log_err("workqueue init error:%d", ret);
		goto err;
	}

	ctx->evloop = awoke_event_loop_create(8);
	if (!ctx->evloop) {
		log_err("evloop create error");
		goto err;
	}

	return ctx;

err:
	mtk_context_release(ctx);
	return NULL;
}

uint64_t mtk_work_service(struct _awoke_minpq *wq, uint64_t nowus)
{
	if (!wq || awoke_minpq_empty(wq))
		return 0;

	log_burst("works:%d", awoke_minpq_size(wq));

	do {
		mtk_work work;
		int priro_nouse;
		awoke_minpq_get(wq, &work, &priro_nouse, 1);
		log_burst("nowus:%lu, us:%lu", nowus, work.us);
		if (work.us > nowus) {
			return (work.us - nowus);
		} else {
			
			awoke_minpq_delmin(wq, &work, &priro_nouse);
			log_trace("work dequeue and sched");
			work.handle(wq);
			return 0;
		}

	} while (1);

	return 0;
}

err_type mtk_service_connect(struct _mtk_context *ctx, struct _mtk_connect *conn)
{
	err_type ret;

	ret = conn->protocol->handle_read(ctx, conn);
	if (ret != et_ok) {
		log_err("read error:%d");
		return ret;
	}

	return ret;
}

err_type mtk_service_run(struct _mtk_context *ctx, uint32_t timeout_ms)
{
	err_type ret;
	mtk_connect *conn;
	awoke_event *event;
	awoke_event_loop *evloop = ctx->evloop;
	uint64_t us, timeout_us;
	
	log_burst("service run");
	
	timeout_us = timeout_ms * 1000;
	
	us = mtk_work_service(&ctx->works_pq, mtk_now_usec());


	awoke_event_wait(evloop, 1000);
	awoke_event_foreach(event, evloop) {

        if (event->type & EVENT_IDLE) {
            continue;
        }
			
		if (event->type == EVENT_CONNECTION) {
			log_debug("event[%d]", event->fd);
			conn = (mtk_connect *)event;
			log_debug("conn->fd:%d", conn->fd);
			ret = mtk_service_connect(ctx, conn);
		}
	}

	
	return et_ok;
}

bool mtk_service_should_stop(struct _mtk_context *ctx)
{
	return ctx->service_need_stop;
}

int build_login_request_pkt(uint8_t *buf, uint8_t version, char *imei_stri, char *slat_stri)
{
	int len;
	uint8_t md5in[128] = {0x0};
	uint8_t md5out[16] = {0x0};
	uint8_t imei_byte[16] = {0x0};
	uint8_t slat_byte[128] = {0x0};
	uint8_t *pos = buf;

	/* iemi encode(BCD8421) */
	awoke_string_str2bcdv2(imei_byte, imei_stri, 16);

	/* slat encode(ASCII) */
	memcpy(slat_byte, slat_stri, strlen(slat_stri));

	/* imei+slat --> md5 */
	memcpy(md5in, imei_byte, 8);
	memcpy(md5in+8, slat_byte, strlen(slat_stri));
	md5(md5in, strlen(slat_stri)+8, md5out);

	/* version */
	pkg_push_byte(version, pos);

	/* imei */
	pkg_push_bytes(imei_byte, pos, 8);
	
	/* md5 */
	pkg_push_bytes(md5out, pos, 16);

	len = pos - buf;
	log_info("imei:%s", imei_stri);
	log_info("salt:%s", slat_stri);
	awoke_hexdump_trace(buf, len);

	return len;
}

static err_type mtk_nc_connect_redirect(struct _mtk_context *ctx, 
	struct _mtk_connect *conn, uint32_t addr, uint16_t port)
{
	uint8_t raddr[4];
	char address[16] = {'\0'};
	mtk_client_connect_info info;

	memcpy(raddr, &addr, 4);
	sprintf(address, "%d.%d.%d.%d", raddr[0], raddr[1], raddr[2], raddr[3]);

	info.context = ctx;
	info.channel = MTK_CHANNEL_NOTICE;
	info.address = address;
	info.port = port;
	
	return mtk_client_connect_via_info(&info);
}

err_type mtk_nc_login(struct _mtk_context *ctx, struct _mtk_connect *conn, 
	struct _mtk_client_connect_info *i)
{
	int n, len;
	uint8_t buf[1024], ret;
	uint32_t raddr = 0;
	uint16_t rport = 0;
	
	len = build_login_request_pkt(buf, 0,
		"0123456789012347", 
		"$%&^4tt2123232@!21@WEEDQ#wewrw3r1eg5$%$%");
	
	log_debug("packet len:%d", len);
	
	n = send(conn->fd, buf, len, 0);
	if (n < 0) {
		log_err("send error");
		close(conn->fd);
		return et_sock_send;
	}

	memset(buf, 0x0, 1024);
	n = read(conn->fd, buf, 1024);
	log_trace("read:%d", n);
	if (n < 0) {
		log_err("read error:%d", errno);
		close(conn->fd);
		return et_sock_recv;
	}

	log_trace("recvdata:");
	awoke_hexdump_trace(buf, n);

	switch (buf[0]) {

	case 0:
		log_info("login fail");
		ret = et_sock_conn;
		goto err;
		break;

	case 1:
		log_info("login success");
		ret = et_ok;
		break;

	case 4:
		log_notice("login redirect(ipv4)");
		conn->need_do_redirect = 1;
		memcpy(&raddr, &buf[1], 4);
		raddr = htonl(raddr);
		memcpy(&rport, &buf[16], 2);
		rport = htons(rport);
		break;

	case 6:
		log_notice("login redirect(ipv6), not support");
		conn->need_do_redirect = 1;
		break;

	default:
		break;
	}

	if (conn->need_do_redirect) {
		close(conn->fd);
		conn->fd = -1;
		conn->need_do_redirect = 0;
		return mtk_nc_connect_redirect(ctx, conn, raddr, rport);
	}
	
	return et_ok;

err:
	close(conn->fd);
	return ret;
}

err_type mtk_tcp_connect(struct _mtk_connect *conn, 
	struct _mtk_client_connect_info *i)
{
	int fd, n;
	char portstr[7] = {'\0'};
	struct addrinfo hint = {0};
	struct addrinfo *res = NULL;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (!fd) {
		log_err("socket create error:%d", errno);
		return et_sock_creat;
	}

	sprintf(portstr, "%d", i->port);
	if (getaddrinfo(i->address, portstr, &hint, &res) != 0) {
		log_err("getaddrinfo errno:%d", errno);
		return et_sock_set;
	}

	n = connect(fd, res->ai_addr, res->ai_addrlen);
	if (n < 0) {
		log_err("socket connect error:%d", errno);
		freeaddrinfo(res);
		close(fd);
		return et_sock_conn;
	}

	freeaddrinfo(res);

	conn->fd = fd;

	return et_ok;
}

void mtk_connect_release(struct _mtk_connect **c)
{
	mtk_connect *p;
	
	if (!*c || !c)
		return;

	p = *c;

	if (p->fd > 0) {
		close(p->fd);
	}

	if (p->ring) {
		awoke_queue_free(&p->ring);
	}
}

err_type mtk_connect_write(struct _mtk_connect *conn, uint8_t *buf, size_t len)
{
	int n;

	n = send(conn->event.fd, buf, len, 0);
	if (n < 0) {
		log_err("send error:%d", errno);
		return et_sock_send;
	}

	return et_ok;
}

err_type mtk_client_connect_via_info(struct _mtk_client_connect_info *i)
{
	err_type ret;
	char chinfo[8] = {'\0'};
	mtk_context *ctx = i->context;
	mtk_connect *conn = &ctx->connects[i->channel];

	log_debug("channel:%d", i->channel);
	conn->chntype = i->channel;
	
	if (i->channel == MTK_CHANNEL_DATA) {
		sprintf(chinfo, "DATA");
	} else {
		sprintf(chinfo, "NOTICE");
		conn->protocol = &proto_light_link;
	}

	log_info("%s:%d channel:%s protocol:%s", i->address, i->port, 
		chinfo, conn->protocol->name);

	ret = mtk_tcp_connect(conn, i);
	if (ret != et_ok) {
		log_err("TCP connect error:%d", ret);
		return ret;
	}

	if (i->channel == MTK_CHANNEL_NOTICE) {
		ret = mtk_nc_login(i->context, conn, i);
		if (ret != et_ok) {
			log_err("login error:%d", ret);
			goto err;
		}
	}

	conn->event.fd = conn->fd;
	conn->event.type = EVENT_CONNECTION;
	conn->event.mask = EVENT_EMPTY;
	conn->event.status = EVENT_NONE;
	ret = awoke_event_add(ctx->evloop, conn->fd,
		EVENT_CONNECTION, EVENT_READ, conn);
	if (ret != et_ok) {
		log_err("register connect fd error:%d", ret);
		goto err;
	}
	log_debug("event[%d] add to evloop", conn->fd);
	conn->ring = awoke_queue_create(sizeof(mtk_notice_msg), 8, AWOKE_QUEUE_F_RB);
	conn->context = ctx;

	return et_ok;
	
err:
	mtk_connect_release(&conn);
	return ret;
}

err_type mtk_work_schedule(struct _mtk_context *ctx, 
	err_type (*cb)(struct _awoke_minpq *q), uint64_t us)
{
	uint64_t usec = us + mtk_now_usec();
	log_trace("work enqueue");
	mtk_work work;
	work.handle = cb;
	work.us = usec;
	return awoke_minpq_insert(&ctx->works_pq, &work, 0);
}

err_type mtk_retry_work_schedule(struct _mtk_context *ctx, 
	err_type (*cb)(struct _awoke_minpq *q), uint64_t us)
{
	return et_ok;
}