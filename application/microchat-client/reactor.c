
#include "client.h"
#include "reactor.h"
#include "awoke_log.h"
#include "awoke_package.h"
#include "server.h"


static struct _awoke_worker *reactor;
static struct _client_vhost_data *vhost_data;


err_type _msg_send(struct _client_vhost_data *vhd, struct msg *m)
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
			lws_cancel_service(vhd->lwsctx);
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
	uint8_t *pos;
	uint16_t header = MC_HEADER;
	struct msg message;
	int len = 128;

	vhost_data->state = CLIENT_STATE_HANDSHARK;

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

static bool header_check(void *d, size_t len)
{
	uint8_t *pos;
	uint16_t header;

	pos = (uint8_t *)d;

	pkg_pull_word(header, pos);
	header = htons(header);

	return (header == MC_HEADER);
}

static void
client_receive_process(void *d, size_t len)
{
	uint8_t type, *pos = (uint8_t *)d;
	
	if (!header_check(d, len)) {
		log_err("header invaild");
		return;
	}

	pos += 2;
	pkg_pull_byte(type, pos);

	switch (vhost_data->state) {

	case CLIENT_STATE_NONE:
		log_trace("state:none");
		break;

	case CLIENT_STATE_HANDSHARK:
		log_trace("state:handshark");
		if (type == mc_handshark) {
			log_debug("handshark ok");
		}
		vhost_data->state = CLIENT_STATE_CONNECTED;
		break;

	case CLIENT_STATE_CONNECTED:
		log_trace("state:connected");
		break;

	default:
		break;
	}
}

static void
connect_client(lws_sorted_usec_list_t *sul)
{
	struct _client_vhost_data *vhd = 
		lws_container_of(sul, client_vhost_data, sul);
	
	vhd->connect.context = vhd->lwsctx;
	vhd->connect.port = 7681;
	vhd->connect.path = "/publisher";
	vhd->connect.address = "localhost";
	vhd->connect.host = vhd->connect.address;
	vhd->connect.origin = vhd->connect.address;
	vhd->connect.ssl_connection = 0;
	vhd->connect.protocol = "micro-chat";
	vhd->connect.pwsi = &vhd->lwswsi;

	if (!lws_client_connect_via_info(&vhd->connect))
		lws_sul_schedule(vhd->lwsctx, 0, &vhd->sul,
				 connect_client, 10 * LWS_US_PER_SEC);
}

static int
callback_micro_chat(struct lws *wsi, enum lws_callback_reasons reason,
		 void *user, void *in, size_t len)
{
	struct _client_vhost_data *vhd =
		(struct _client_vhost_data *)
		lws_protocol_vh_priv_get(lws_get_vhost(wsi),
				lws_get_protocol(wsi));
	const struct msg *pmsg;
	int m;
	
	switch (reason) {

	case LWS_CALLBACK_PROTOCOL_INIT:
		log_trace("PROTOCOL_INIT");
		vhd = lws_protocol_vh_priv_zalloc(lws_get_vhost(wsi),
				lws_get_protocol(wsi),
				sizeof(struct _client_vhost_data));
		vhd->lwsctx = lws_get_context(wsi);
		vhd->protocol = lws_get_protocol(wsi);
		vhd->lwsvhost = lws_get_vhost(wsi);
		vhd->ring = lws_ring_create(sizeof(struct msg), 8,
					    destroy_message);
		if (!vhd->ring) {log_err("ring create error");return 1;}
		pthread_mutex_init(&vhd->lock_ring, NULL);
		connect_client(&vhd->sul);
		vhost_data = vhd;
		break;
		
	case LWS_CALLBACK_PROTOCOL_DESTROY:
		log_trace("PROTOCOL_DESTROY");
		if (vhd->ring) {
			lws_ring_destroy(vhd->ring);
		}
		
		lws_sul_cancel(&vhd->sul);
		
		pthread_mutex_destroy(&vhd->lock_ring);
		return 0;

	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
		log_err("CLIENT_CONNECTION_ERROR: %s\n",
			 in ? (char *)in : "(null)");
		vhd->lwswsi = NULL;
		lws_sul_schedule(vhd->lwsctx, 0, &vhd->sul,
				 connect_client, LWS_US_PER_SEC);
		break;

	case LWS_CALLBACK_CLIENT_ESTABLISHED:
		log_trace("CLIENT_ESTABLISHED");
		vhd->established = 1;
		break;

	case LWS_CALLBACK_CLIENT_WRITEABLE:
		log_trace("CLIENT_WRITEABLE");
		pthread_mutex_lock(&vhd->lock_ring); /* --------- ring lock { */
		pmsg = lws_ring_get_element(vhd->ring, &vhd->ring_tail);
		if (!pmsg)
			goto skip;

		m = lws_write(wsi, ((unsigned char *)pmsg->payload) + LWS_PRE,
					  pmsg->len, LWS_WRITE_TEXT);
		if (m < (int)pmsg->len) {
			pthread_mutex_unlock(&vhd->lock_ring); /* } ring lock */
			log_err("ERROR %d writing to ws socket\n", m);
			return -1;
		}

		lws_ring_consume_single_tail(vhd->ring, &vhd->ring_tail, 1);
		if (lws_ring_get_element(vhd->ring, &vhd->ring_tail)) {
			lws_callback_on_writable(wsi);
		}
skip:
		pthread_mutex_unlock(&vhd->lock_ring); /* } ring lock ------- */
		break;

	case LWS_CALLBACK_CLIENT_RECEIVE:
		log_trace("RECEIVE");
		client_receive_process(in, len);
		break;

	case LWS_CALLBACK_CLIENT_CLOSED:
		log_trace("CLIENT_CLOSED");
		vhd->lwswsi = NULL;
		vhd->established = 0;
		lws_sul_schedule(vhd->lwsctx, 0, &vhd->sul,
				 connect_client, LWS_US_PER_SEC);
		break;

	case LWS_CALLBACK_EVENT_WAIT_CANCELLED:
		log_trace("EVENT_WAIT_CANCELLED");
		if (vhd && vhd->lwswsi && vhd->established)
			lws_callback_on_writable(vhd->lwswsi);

	default:
		break;
	}

	return lws_callback_http_dummy(wsi, reason, user, in, len);
}

static const struct lws_protocols protocols[] = {
	{ "micro-chat", callback_micro_chat, 0, 0, },
	{ NULL, NULL, 0, 0 }
};

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
	struct lws_context *context;
	struct lws_context_creation_info info;
	struct _awoke_worker_context *ctx = data;
	struct _awoke_worker *worker = ctx->worker;

	log_trace("worker:%s start", worker->name);
	reactor = worker;

	memset(&info, 0, sizeof info);
	info.port = CONTEXT_PORT_NO_LISTEN;
	info.protocols = protocols;
	info.fd_limit_per_thread = 1 + 1 + 1;

	context = lws_create_context(&info);
	if (!context) {
		log_err("lws init failed");
		return et_fail;
	}

	do {

		ret = reactor_poll(context);
	
	} while (!awoke_worker_should_stop(worker) &&
			 (ret == et_ok));

	lws_context_destroy(context);

	log_notice("%s stop", worker->name);

	return ret;
}
