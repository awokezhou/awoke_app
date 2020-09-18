
#include "micro_talk.h"
#include "mtk_session.h"
#include "awoke_package.h"
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
err_type light_link_read(struct _mtk_connect *conn);
err_type light_link_write(struct _mtk_connect *conn);
err_type short_link_read(struct _mtk_connect *conn);


static struct _mtk_protocol_handle proto_light_link = {
	.name = "LightLink",
	.handle_read = light_link_read,
	.handle_write = light_link_write,
};

static struct _mtk_protocol_handle proto_short_link = {
	.name = "ShortLink",
	.handle_read = short_link_read,
	.handle_write = light_link_write,
};

err_type short_link_read(struct _mtk_connect *conn)
{
	int n;
	int max_read;
	int need_read;
	int available = 0;
	uint8_t *pos, code;
	uint16_t length;
	err_type ret;
	awoke_buffchunk *chunk;
	mtk_context *ctx = conn->context;
	awoke_event *event = &conn->event;
	
	if (event->mask & EVENT_WRITE)
		return mtk_handle_write_event(ctx, conn);

	/*
	 * -- Protocol Receive Context --
	 * 
	 *    First we use a default rx buffer to receive data, 
	 */


	chunk = ctx->rx_buff;
	need_read = chunk->size - chunk->length;

retry_read:

	available = chunk->size - chunk->length;
	if (available < need_read) {
		if (!conn->spare_buff) {
			conn->spare_buff = awoke_buffchunk_create(need_read+chunk->length);
			if (!conn->spare_buff) {
				log_err("create spare buffer error");
				return et_nomem;
			}
			awoke_buffchunk_dump(conn->spare_buff);
			memcpy(conn->spare_buff->p, chunk->p, chunk->length);
			conn->spare_buff->length = chunk->length;
			chunk = conn->spare_buff;
		} else {
			ret = awoke_buffchunk_resize(chunk, need_read+chunk->length, FALSE);
			if (ret != et_ok) {
				log_err("resize spare buffer error");
				awoke_buffchunk_free(&conn->spare_buff);
				return et_nomem;
			}
		}
	}

	max_read = chunk->size - chunk->length;
	log_trace("maxread:%d", max_read);
	n = recv(event->fd, chunk->p + chunk->length, max_read, 0);
	log_trace("fd[%d] read %d", event->fd, n);

	if (!n) {
		log_err("fd[%d] broken", event->fd);
		return et_sock_recv;
	} else if (n < 0) {
		log_err("fd[%d] error:%d", event->fd, errno);
		return et_sock_recv;
	}

	if (n > max_read) {
		need_read = n - max_read;
		log_trace("fd[%d] still data %d", event->fd, need_read);
		chunk->length += max_read;
		goto retry_read;
	} else {
		
		chunk->length += n;
		log_trace("chunk length:%d", chunk->length);
		
		pos = (uint8_t *)chunk->p;
		pkg_pull_byte(code, pos);
		pkg_pull_word(length, pos);
		length = htons(length);
		log_trace("data length:%d", length);
		
		if ((length + MTK_RSP_HEADER_LENGTH) > chunk->length) {
			need_read = length + MTK_RSP_HEADER_LENGTH - chunk->length;
			log_trace("fd[%d] still data %d", event->fd, need_read);
			goto retry_read;
		}

		log_trace("receive callback length %d", chunk->length);
		ret = conn->callback(conn, MTK_CALLBACL_CONNECTION_RECEIVE, 
			conn->user, chunk->p, chunk->length);
	}

	/*
	if (conn->spare_buff)
		awoke_buffchunk_free(&conn->spare_buff);
*/
	if (!conn->f_keepalive) {
		log_debug("connect[%d] not support keepalive, release it", conn->chntype);
		mtk_connect_release(conn);
	}

	return ret;
}

err_type light_link_read(struct _mtk_connect *conn)
{
	int n;
	err_type ret;
	awoke_buffchunk *chunk;
	mtk_context *ctx = conn->context;
	
	if (conn->event.mask & EVENT_WRITE) {
		return mtk_handle_write_event(ctx, conn);
	}

	/* read data */
	chunk = ctx->rx_buff;

	if (!awoke_buffchunk_remain(chunk)) {
		log_err("some data need to be read");
		ret = conn->callback(conn, MTK_CALLBACL_CONNECTION_RECEIVE, 
			conn->user, chunk->p, chunk->length);
	} else {

		n = recv(conn->event.fd, chunk->p, chunk->size, 0);
		if (n < 0) {
			log_err("receive error:%d", errno);
			return et_sock_recv;
		}
		chunk->length = n;
		log_trace("fd:%d read length:%d", conn->fd, n);
		ret = conn->callback(conn, MTK_CALLBACL_CONNECTION_RECEIVE, 
			conn->user, chunk->p, chunk->length);
	}
	
	return ret;
}

err_type light_link_write(struct _mtk_connect *conn)
{
	return et_ok;
}

bool mtk_file_exist(const char *filepath)
{
	FILE *fp;

	fp = fopen(filepath, "r");
	if (!fp) {
		return FALSE;
	}

	fclose(fp);
	return TRUE;
}

uint16_t mtk_getbuffcrc(char *buff, int totalsize)
{
	return awoke_crc16((uint8_t *)buff, totalsize);
}

uint16_t mtk_getfilecrc(const char *filepath, int filesize)
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

int mtk_getfilesize(const char *filepath)
{
	struct stat statbuff;

	if (stat(filepath, &statbuff) < 0) {
		return -1;
	} else {
		return statbuff.st_size;
	}
}
uint64_t mtk_now_usec(void)
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
	uint32_t prevmask = e->mask;
	awoke_event_del(evl, e);
	e->mask = (prevmask & ~and) | or;
	awoke_event_add(evl, e->fd, e->type, e->mask, data);
	log_burst("event[%d] 0x%x->0x%x", e->fd, prevmask, e->mask);
}

err_type mtk_callback_on_writable(struct _mtk_connect *conn)
{
	mtk_event_change_mask(conn->context->evloop, &conn->event, 0x0, EVENT_WRITE, conn);
	return et_ok;
}

err_type mtk_callback_as_writeable(struct _mtk_connect *conn)
{
	return conn->callback(conn, MTK_CALLBACL_CONNECTION_WRITABLE, conn->user, NULL, 0);
}

err_type mtk_handle_write_event(struct _mtk_context *ctx, struct _mtk_connect *conn)
{
	err_type ret;
	
	ret = conn->protocol->handle_write(conn);
	if (ret != et_ok) {
		log_err("protocol handle write error:%d", ret);
		return ret;
	}

	mtk_event_change_mask(ctx->evloop, &conn->event, EVENT_WRITE, 0x0, conn);
	
	return mtk_callback_as_writeable(conn);
}

struct _mtk_context *mtk_context_create(const char *name, const char *addr, int dp, int np, 
	const char *imei)
{
	err_type ret;
	
	mtk_context *ctx = mem_alloc_trace(sizeof(mtk_context), "mtk context");
	if (!ctx) {
		log_err("mtk context create error");
		return NULL;
	}

	memset(ctx, 0x0, sizeof(mtk_context));

	ctx->service_need_stop = FALSE;

	ctx->session_baseid = 0;

	ctx->server_address = (char *)addr;
	ctx->dc_port = dp;
	ctx->nc_port = np;
	ctx->imei = (char *)imei;

	if (!name)
		ctx->name = awoke_string_dup("micro-talk");
	else
		ctx->name = awoke_string_dup(name);

	ret = awoke_minpq_init(&ctx->works_pq, sizeof(mtk_work), 8, 
		work_us_comparator, 0x0);
	if (ret != et_ok) {
		log_err("workqueue init error:%d", ret);
		goto err;
	}

	log_trace("work queue size:%d", ctx->works_pq.capacity);

	ctx->evloop = awoke_event_loop_create(8);
	if (!ctx->evloop) {
		log_err("evloop create error");
		goto err;
	}

	log_trace("event loop size:%d", ctx->evloop->size);

	ctx->rx_buff = awoke_buffchunk_create(4096);
	if (!ctx->rx_buff) {
		log_err("rxbuff create error");
		goto err;
	}

	awoke_buffchunk_dump(ctx->rx_buff);
	
	return ctx;

err:
	mtk_context_release(ctx);
	return NULL;
}

uint64_t mtk_service_work(struct _mtk_context *ctx, uint64_t nowus)
{
	awoke_minpq *wq = &ctx->works_pq;
	
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
			log_burst("work dequeue and sched");
			work.handle(ctx, &work);
			return 0;
		}

	} while (1);

	return 0;
}

err_type mtk_service_connect(struct _mtk_context *ctx, struct _mtk_connect *conn)
{
	err_type ret;

	ret = conn->protocol->handle_read(conn);
	if (ret != et_ok) {
		log_err("read error:%d");
		return ret;
	}

	if (conn->f_rwait_timeout) {
		conn->rwait.wait_us = 0;
		conn->rwait.timeout_cnt = 0;
	}

	memset(ctx->rx_buff->p, 0x0, ctx->rx_buff->size);
	ctx->rx_buff->length = 0;

	return ret;
}

static err_type mtk_service_adjust_timeout(struct _mtk_context *ctx)
{
	int i;
	err_type ret;
	mtk_connect *c;
	uint64_t us = mtk_now_usec();

	for (i=0; i<MTK_CHANNEL_SIZEOF; i++) {

		c = &ctx->connects[i];
		if (c->f_connected) {

			if (c->f_rwait_timeout) {
				if ((c->rwait.wait_us > 0) && (c->rwait.wait_us <= us)) {
					log_err("connect[%d] rwait timeout", i);
					c->rwait.timeout_cnt++;
					if (c->rwait.timeout_cnt >= c->rwait.maxcnt) {
						log_err("connect[%d] rwait upto max %d", c->rwait.maxcnt);
						mtk_connect_close(c);
						return et_sock_recv;
					}
					ret = c->callback(c, MTK_CALLBACK_CONNECTION_RECEIVE_TIMEOUT, 
						c->user, NULL, 0);
					if (ret != et_ok) {
						log_err("callback error:%d", ret);
						mtk_connect_close(c);
						return et_sock_recv;
					}
				}
			}
		}
	}
	
	return et_ok;
}

err_type mtk_service_run(struct _mtk_context *ctx, uint32_t timeout_ms)
{
	err_type ret;
	mtk_connect *conn;
	awoke_event *event;
	awoke_event_loop *evloop = ctx->evloop;
	uint64_t us, timeout_us;


	/* if timeout not set, give a default timeout with 2 second */
	if (timeout_ms <= 0)
		//timeout_ms = 2 * MTK_MS_PER_SEC;
		timeout_ms = 100;
	
	timeout_us = timeout_ms * MTK_US_PER_MS;

	/* schedule works in workqueue */
	us = mtk_service_work(ctx, mtk_now_usec());
	if (us && (us<timeout_us)) {
		timeout_us = us;
		log_burst("update timeout:%d(us)", timeout_us);
	}

	/* is there pending things that needs service forcing? */
	ret = mtk_service_adjust_timeout(ctx);
	if (ret != et_ok) {
		timeout_us = 0;
	}


	/* 
	 * IO multiplexing event loop
	 */
	awoke_event_wait(evloop, timeout_us/MTK_US_PER_MS);
	
	awoke_event_foreach(event, evloop) {

        if (event->type & EVENT_IDLE)
            continue;

		log_trace("event[%d] 0x%x 0x%x", event->fd, event->type, event->mask);
			
		if (event->type == EVENT_CONNECTION) {
			
			conn = (mtk_connect *)event;
			
			ret = mtk_service_connect(ctx, conn);
			if (ret != et_ok) {
				log_err("event[%d] service error:%d", ret);
				mtk_connect_close(conn);
			}
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
	log_trace("imei:%s", imei_stri);
	log_trace("salt:%s", slat_stri);

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

	log_trace("login start");
	
	len = build_login_request_pkt(buf, 0,
		ctx->imei, 
		"$%&^4tt2123232@!21@WEEDQ#wewrw3r1eg5$%$%");

	log_trace("login request(%d):", len);
	awoke_hexdump_trace(buf, len);
		
	n = send(conn->fd, buf, len, 0);
	if (n < 0) {
		log_err("send error");
		close(conn->fd);
		return et_sock_send;
	}

	memset(buf, 0x0, 1024);
	n = read(conn->fd, buf, 1024);
	if (n < 0) {
		log_err("read error:%d", errno);
		close(conn->fd);
		return et_sock_recv;
	}

	log_trace("login response(%d):", n);
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

err_type mtk_tcp_connect(struct _mtk_connect *conn)
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

	sprintf(portstr, "%d", conn->port);
	if (getaddrinfo(conn->address, portstr, &hint, &res) != 0) {
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

bool mtk_connect_keepalive(struct _mtk_connect *c)
{
	if ((c->ka_timeout_us > 0) || (c->f_keepalive == 1))
		return TRUE;
	else
		return FALSE;
}

void mtk_connect_rwait_update(struct _mtk_connect *c)
{
	if (c->f_rwait_timeout && c->rwait.timeout_us) {
		c->rwait.wait_us = mtk_now_usec() + c->rwait.timeout_us;
		log_trace("rwaite update %llu", c->rwait.wait_us);
	}
}

void mtk_connect_rwait_clean(struct _mtk_connect *c)
{
	if (c->f_rwait_timeout) {
		c->f_rwait_timeout = 0;
		memset(&c->rwait, 0x0, sizeof(mtk_recv_wait));
	}
}

void mtk_connect_rwait_init(struct _mtk_connect *c, struct _mtk_client_connect_info *i)
{
	if (i->recv_timout_us) {
		c->f_rwait_timeout = 1;
		c->rwait.wait_us = 0;
		c->rwait.timeout_cnt = 0;
		c->rwait.timeout_us = i->recv_timout_us;
		c->rwait.maxcnt = i->recv_timout_maxcnt;
		log_trace("rwaite init timeout:%llu %d", 
			c->rwait.timeout_us, c->rwait.maxcnt);
	}
}

void mtk_connect_release(struct _mtk_connect *c)
{
	mtk_context *ctx;
	
	if (!c)
		return;

	ctx = c->context;

	c->callback(c, MTK_CALLBACK_CONNECTION_CLOSED, c->user, NULL, 0);

	c->f_connected = 0;
	c->f_keepalive = 0;
	c->need_do_redirect = 0;

	mtk_connect_rwait_clean(c);

	awoke_event_del(ctx->evloop, &c->event);
	log_trace("event[%d] remove", c->event.fd);
	
	if (c->fd > 0) {
		close(c->fd);
		log_trace("fd[%d] close", c->event.fd);
		c->fd = -1;
	}

	if (c->ring) {
		awoke_queue_free(&c->ring);
		c->ring = NULL;
	}

	if (c->spare_buff) {
		awoke_buffchunk_free(&c->spare_buff);
		c->spare_buff = NULL;
	}

	if (c->session) {
		if (c->session->state == MTK_SESSION_FIN) {
			log_trace("session[%d] free", c->session->id);
			mtk_session_free(c->session);
			c->session = NULL;
		}		
	}

	log_debug("connect[%d] release\n", c->chntype);
 }

err_type mtk_connect_write(struct _mtk_connect *conn, uint8_t *buf, size_t len)
{
	int n;

	log_trace("fd[%d] write length:%d", conn->fd, len);
	n = send(conn->event.fd, buf, len, 0);
	if (n < 0) {
		log_err("send error:%d", errno);
		return et_sock_send;
	}

	if (conn->f_rwait_timeout) {
		mtk_connect_rwait_update(conn);
	}
	
	return et_ok;
}

err_type mtk_connect_read(struct _mtk_connect *conn, uint8_t *buf, size_t len)
{
	int n=0;
	uint8_t *start;

	log_trace("need read:%d", len);

	while (n != len) {

		start = buf + n;
		n += recv(conn->event.fd, start, len, 0);
		if (n < 0) {
			log_err("send error:%d", errno);
			return et_sock_recv;
		}
		log_trace("fd[%d] read length:%d", conn->fd, n);
	}

	return et_ok;
}

void mtk_connect_close(struct _mtk_connect *conn)
{
	if (conn->protocol->handle_close)
		conn->protocol->handle_close(conn);

	return mtk_connect_release(conn);
}

err_type mtk_client_connect_via_info(struct _mtk_client_connect_info *i)
{
	err_type ret;
	char chinfo[8] = {'\0'};
	mtk_context *ctx = i->context;
	mtk_connect *conn = &ctx->connects[i->channel];


	if (conn->f_connected) {
		log_debug("connect[%d] already connected", i->channel);
		return et_ok;
	}
	
	conn->port = i->port;
	conn->address = i->address;
	conn->chntype = i->channel;
	conn->callback = i->callback;
	
	conn->ka_timeout_us = i->ka_timeout_us;
	if (conn->ka_timeout_us) 
		conn->f_keepalive = 1;

	mtk_connect_rwait_init(conn, i);
	
	if (i->channel == MTK_CHANNEL_DATA) {
		sprintf(chinfo, "DATA");
		conn->protocol = &proto_short_link;
	} else {
		sprintf(chinfo, "NOTICE");
		conn->protocol = &proto_light_link;
	}

	log_trace("");
	log_trace("connect[%d] informaion:", i->channel);
	log_trace("------------------------");
	log_trace("address:%s:%d", i->address, i->port);
	log_trace("channel:%s", chinfo);
	log_trace("protocol:%s", conn->protocol->name);
	if (conn->f_keepalive) {
		log_trace("ka timeout:%d(us)", conn->ka_timeout_us);
	}
	if (conn->f_rwait_timeout) {
		log_trace("recv timeout:%d(us)", conn->rwait.timeout_us);
	}
	log_trace("------------------------");
	log_trace("");

	log_debug("connecting...");

	conn->callback(conn, MTK_CALLBACK_PROTOCOL_INIT, conn->user, NULL, 0);

	ret = mtk_tcp_connect(conn);
	if (ret != et_ok) {
		log_err("TCP connect error:%d", ret);
		goto err;
	}

	log_debug("connected");

	if (i->channel == MTK_CHANNEL_NOTICE) {
		ret = mtk_nc_login(i->context, conn, i);
		if (ret != et_ok) {
			log_err("login error:%d", ret);
			goto err;
		}
	}

	log_debug("connect[%d] established", i->channel);

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
	log_trace("event[%d] add to evloop", conn->fd);
	conn->context = ctx;
	
	conn->callback(conn, MTK_CALLBACK_CONNECTION_ESTABLISHED, conn->user, NULL, 0);
	conn->f_connected = 1;
	return et_ok;
	
err:
	mtk_connect_release(conn);
	return ret;
}

err_type mtk_work_schedule_withdata(struct _mtk_context *ctx, 
	err_type (*cb)(struct _mtk_context *, struct _mtk_work *), uint64_t us, void *data)
{
	mtk_work work;
	work.handle = cb;
	work.data = data;
	work.us = us + mtk_now_usec();
	log_burst("work enqueue");
	return awoke_minpq_insert(&ctx->works_pq, &work, 0);
}

err_type mtk_work_schedule(struct _mtk_context *ctx, 
	err_type (*cb)(struct _mtk_context *, struct _mtk_work *), uint64_t us)
{
	uint64_t usec = us + mtk_now_usec();
	mtk_work work;
	work.handle = cb;
	work.us = usec;
	log_burst("work enqueue");
	return awoke_minpq_insert(&ctx->works_pq, &work, 0);
}

err_type mtk_work_delete(struct _mtk_context *ctx, 
	err_type (*cb)(struct _mtk_context *, struct _mtk_work *))
{
	bool find = FALSE;
	int i, size, prior;
	mtk_work work;

	log_trace("cb:0x%x", cb);
	size = awoke_minpq_size(&ctx->works_pq);

	if (!size)
		return et_ok;

	for (i=1; i<=size; i++) {
		awoke_minpq_get(&ctx->works_pq, &work, &prior, i);
		if (work.handle == cb) {
			log_trace("find %d", i);
			find = TRUE;
			break;
		}
	}

	if (find) {
		awoke_minpq_del(&ctx->works_pq, &work, &prior, i);
	}

	return et_ok;
}

err_type mtk_retry_work_schedule(struct _mtk_context *ctx, 
	err_type (*cb)(struct _mtk_context *, struct _mtk_work *), uint64_t us)
{
	return et_ok;
}

err_type mtk_connect_session_add(struct _mtk_connect *c, struct _mtk_session *s)
{
	c->session = s;
	return et_ok;
}

err_type mtk_connect_session_del(struct _mtk_connect *c)
{
	if (!c->session)
		return et_ok;
	mtk_session_free(c->session);
	c->session = NULL;
	return et_ok;
}

err_type mtk_chunkinfo_generate_from_buff(char *buff, int len, 
	int chunksize, struct _mtk_chunkinfo **r)
{
	mtk_chunkinfo *ci;

	if (!buff || len<0 || chunksize<0)
		return et_param;

	ci = mem_alloc_trace(sizeof(mtk_chunkinfo), "chunkinfo");
	if (!ci)
		return et_nomem;

	ci->buffin.p = buff;
	ci->buffin.size = len;
	ci->buffin.length = len;

	ci->totalsize = len;

	ci->crc = mtk_getbuffcrc(buff, ci->totalsize);
	if (!ci->crc) {
		log_err("get buff CRC error");
		return et_param;
	}

	ci->hash = 6;
	ci->index = 0;

	if (ci->totalsize <= chunksize)
		ci->chunknum = 1;
	else
		ci->chunknum = (ci->totalsize % chunksize) ? (ci->totalsize/chunksize+1) : ci->totalsize/chunksize;
	ci->chunksize = ci->totalsize <= chunksize ? ci->totalsize : chunksize;

	*r = ci;

	log_trace("");
	log_trace("buffchunk");
	log_trace("----------------");
	log_trace("address:0x%x", buff);
	log_trace("totalsize:%d(bytes)", ci->totalsize);
	log_trace("chunknum:%d", ci->chunknum);
	log_trace("chunksize:%d", ci->chunksize);
	log_trace("HASH:0x%x", ci->hash);
	log_trace("CRC:0x%x", ci->crc);
	log_trace("----------------");
	log_trace("");

	return et_ok;
}

err_type mtk_chunkinfo_generate_from_file(const char *filepath, 
	int chunksize, struct _mtk_chunkinfo **r)
{
	mtk_chunkinfo *ci;
	
	if (!filepath || (chunksize<0))
		return et_param;

	if (!mtk_file_exist(filepath)) {
		log_err("file not exist");
		return et_file_open;
	}

	ci = mem_alloc_trace(sizeof(mtk_chunkinfo), "chunkinfo");
	if (!ci) {
		log_err("alloc chunkinfo error");
		return et_nomem;
	}
	memset(ci, 0x0, sizeof(mtk_chunkinfo));

	ci->filepath = awoke_string_dup(filepath);

	ci->totalsize = mtk_getfilesize(filepath);
	if (ci->totalsize <= 0) {
		log_err("get file size error");
		return et_file_open;
	}

	ci->crc = mtk_getfilecrc(filepath, ci->totalsize);
	if (!ci->crc) {
		log_err("get file CRC error");
		return et_param;
	}

	ci->hash = 6;
	ci->index = 0;
	if (ci->totalsize <= chunksize)
		ci->chunknum = 1;
	else
		ci->chunknum = (ci->totalsize % chunksize) ? (ci->totalsize/chunksize+1) : ci->totalsize/chunksize;
	ci->chunksize = ci->totalsize <= chunksize ? ci->totalsize : chunksize;

	*r = ci;

	log_debug("");
	log_debug("filechunk");
	log_debug("----------------");
	log_debug("filepath:%s", filepath);
	log_debug("filesize:%d(bytes)", ci->totalsize);
	log_debug("chunknum:%d", ci->chunknum);
	log_debug("chunksize:%d", ci->chunksize);
	log_debug("HASH:0x%x", ci->hash);
	log_debug("CRC:0x%x", ci->crc);
	log_debug("----------------");
	log_debug("");

	return et_ok;
}

err_type mtk_filechunk_write(const char *filepath, char *p, int len)
{
	int n;
	FILE *fp;

	fp = fopen(filepath, "ab+");
	if (!fp) {
		log_err("open file error");
		return et_file_open;
	}

	n = fwrite(p, len, 1, fp);
	if (n <= 0) {
		log_err("write error");
		fclose(fp);
		return et_file_send;
	}

	fclose(fp);

	return et_ok;	
}

err_type mtk_filchunk_merge(struct _mtk_chunkinfo *ci)
{
	int prior;
	err_type ret = et_ok;
	awoke_buffchunk chunk;
	char filepath[32] = {'\0'};

	if (!ci->filepath) {
		log_debug("use default");
		sprintf(filepath, "mtk-default-chunkfile");
	} else {
		log_debug("use ci %s", ci->filepath);
		sprintf(filepath, "%s", ci->filepath);
	}

	if (mtk_file_exist(filepath)) {
		log_debug("remove %s", filepath);
		remove(filepath);
	}

	log_info("file %d save to %s", ci->fileid, filepath);

	while (!awoke_minpq_empty(ci->chunk_pq)) {

		awoke_minpq_delmin(ci->chunk_pq, &chunk, &prior);
		ret = mtk_filechunk_write(filepath, chunk.p, chunk.length);
		if (ret != et_ok) {
			log_err("write %s error:%d", filepath, ret);
			return ret;
		}
		
		log_trace("write %d bytes", chunk.length);
		mem_free(chunk.p);
	}

	return et_ok;
}

err_type mtk_filchunk_copy(struct _mtk_chunkinfo *ci, uint8_t *dst, uint32_t start, int size)
{
	int n;
	FILE *fp;

	fp = fopen(ci->filepath, "rb");
	if (!fp) {
		log_err("file open error");
		return et_file_open;
	}

	fseek(fp, start, SEEK_SET);

	n = fread(dst, 1, size, fp);
	if (n != size) {
		if (n < 0) {
			log_err("file read error");
			fclose(fp);
			return et_file_read;
		}
	}

	fclose(fp);
	
	return et_ok;
}

err_type mtk_buffchunk_copy(struct _mtk_chunkinfo *ci, uint8_t *dst, uint32_t start, int size)
{
	memcpy(dst, ci->buffin.p+start, size);
	return et_ok;
}

err_type mtk_chunkdata_copy(struct _mtk_chunkinfo *ci, uint8_t *dst, uint32_t start, int size)
{
	if (ci->type == MTK_CHUNKTYPE_FILE)
		return mtk_filchunk_copy(ci, dst, start, size);
	else if (ci->type == MTK_CHUNKTYPE_BUFF)
		return mtk_buffchunk_copy(ci, dst, start, size);
	return et_fail;	
}

err_type mtk_buffchunk_merge(struct _mtk_chunkinfo *ci)
{
	int prior;
	err_type ret = et_ok;
	awoke_buffchunk chunk;
	
	if (ci->buffout) {
		awoke_buffchunk_free(&ci->buffout);
	}

	ci->buffout = mem_alloc_trace(ci->totalsize, "buffout");
	if (!ci->buffout) 
		return et_nomem;

	while (!awoke_minpq_empty(ci->chunk_pq)) {

		awoke_minpq_delmin(ci->chunk_pq, &chunk, &prior);
		awoke_buffchunk_write(ci->buffout, chunk.p, chunk.length, TRUE);
		if (ret != et_ok) {
			log_err("write error:%d", ret);
			return ret;
		}

		log_trace("write %d bytes", chunk.length);
		mem_free(chunk.p);
	}

	return et_ok;
}

err_type mtk_chunkdata_merge(struct _mtk_chunkinfo *ci)
{
	if (ci->type == MTK_CHUNKTYPE_FILE)
		return mtk_filchunk_merge(ci);
	else if (ci->type == MTK_CHUNKTYPE_BUFF)
		return mtk_buffchunk_merge(ci);
	return et_fail;
}

void mtk_chunkinfo_free(void *d)
{
	int prior;
	mtk_chunkinfo *ci = d;
	awoke_buffchunk chunk;

	log_trace("chunkinfo free");
	
	if (ci->type == MTK_CHUNKTYPE_FILE) {
		if (ci->filepath) {
			mem_free(ci->filepath);
		}
	} else if (ci->type == MTK_CHUNKTYPE_BUFF) {
		if (ci->buffout) {
			awoke_buffchunk_free(&ci->buffout);
		}
	}

	if (ci->chunk_pq) {
		while (!awoke_minpq_empty(ci->chunk_pq)) {
			awoke_minpq_delmin(ci->chunk_pq, &chunk, &prior);
			mem_free(chunk.p);
		}
		awoke_minpq_free(&ci->chunk_pq);
		ci->chunk_pq = NULL;
	}
}
