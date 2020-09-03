
#include "reactor.h"
#include "awoke_log.h"
#include <libwebsockets.h>

#define LWS_PLUGIN_STATIC
#include "protocol_micro_chat.c"


struct lws_context *context;

static struct lws_protocols protocols[] = {
	{ "http", lws_callback_http_dummy, 0, 0 },
	LWS_PLUGIN_PROTOCOL_MICRO_CHAT,
	{ NULL, NULL, 0, 0 } /* terminator */
};

static int interrupted, options;

static const struct lws_protocol_vhost_options pvo_options = {
        NULL,
        NULL,
        "options",              /* pvo name */
        (void *)&options        /* pvo value */
};

static const struct lws_protocol_vhost_options pvo_interrupted = {
        &pvo_options,
        NULL,
        "interrupted",          /* pvo name */
        (void *)&interrupted    /* pvo value */
};

static const struct lws_protocol_vhost_options pvo = {
        NULL,           /* "next" pvo linked-list */
        &pvo_interrupted,       /* "child" pvo linked-list */
        "micro-chat", /* protocol name we belong to on this vhost */
        ""              /* ignored */
};

void destroy_message(void *_msg)
{
	struct msg *msg = _msg;

	free(msg->payload);
	msg->payload = NULL;
	msg->len = 0;
	log_trace("message destory");
}

err_type _msg_send(struct _server_vhost_data *vhd, struct msg *m)
{
	int n;
	
	do {

		if (!vhd->established)
			goto wait;

		pthread_mutex_lock(&vhd->lock_ring); /* --------- ring lock { */

		n = (int)lws_ring_get_count_free_elements(vhd->ring);
		if (!n) {log_err("dropping");goto wait_unlock;}

		n = (int)lws_ring_insert(vhd->ring, m, 1);
		if (n != 1) {
			destroy_message(m);
			log_err("dropping");
		} else {
			lws_callback_on_writable(vhd->lwswsi);
			log_trace("notice callback to send");
			pthread_mutex_unlock(&vhd->lock_ring); /* } ring lock ------- */
			return et_ok;
		}
		
wait_unlock:
		pthread_mutex_unlock(&vhd->lock_ring); /* } ring lock ------- */
wait:
		sleep(5);
	} while (!vhd->finished);

	return et_ok;
}

err_type handshark_msg_send(void)
{
	log_trace("handshark_msg_send");
	uint8_t *pos;
	uint16_t header = MC_HEADER;
	struct msg message;
	int len = 128;

	message.payload = malloc(LWS_PRE + len);
	if (!message.payload) {log_err("OOM:dropping");return et_nomem;}

	pos = message.payload + LWS_PRE;

	message.type = 1;

	/* header */
	header = htons(header);
	pkg_push_word(header, pos);
	
	/* type */
	pkg_push_byte(message.type, pos);

	message.len = pos - (uint8_t *)message.payload + LWS_PRE;
	log_trace("message length:%d", message.len);
	
	awoke_hexdump_trace(message.payload, message.len);

	return _msg_send(vhost_data, &message);
}

err_type reactor_poll(struct lws_context *ctx)
{
	int n;

	n = lws_service(ctx, 0);
	if (n < 0) {
		log_err("lws service error:%d", n);
		return et_fail;
	}

	return et_ok;	
}

err_type reactor_worker(void *data)
{
	err_type ret;
	struct lws_context_creation_info info;
	struct _awoke_worker_context *ctx = data;
	struct _awoke_worker *worker = ctx->worker;
	
	memset(&info, 0, sizeof info);
	info.port = 7681;
	info.protocols = protocols;
	info.pvo = &pvo;
	info.pt_serv_buf_size = 32 * 1024;

	context = lws_create_context(&info);
	if (!context) {
		log_err("lws init failed\n");
		return 1;
	}

	do {

		ret = reactor_poll(context);
	
	} while (!awoke_worker_should_stop(worker) &&
			 (ret == et_ok));

	lws_context_destroy(context);

	log_notice("%s stop", worker->name);

}
