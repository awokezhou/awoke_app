/*
 * ws protocol handler plugin for "lws-minimal-pmd-bulk"
 *
 * Written in 2010-2019 by Andy Green <andy@warmcat.com>
 *
 * This file is made available under the Creative Commons CC0 1.0
 * Universal Public Domain Dedication.
 *
 * The protocol shows how to send and receive bulk messages over a ws connection
 * that optionally may have the permessage-deflate extension negotiated on it.
 */

#if !defined (LWS_PLUGIN_STATIC)
#define LWS_DLL
#define LWS_INTERNAL
#include <libwebsockets.h>
#endif

#include <string.h>
#include "awoke_type.h"
#include "awoke_log.h"
#include "awoke_package.h"
#include "server.h"
#include "reactor.h"

/*
 * We will produce a large ws message either from this text repeated many times,
 * or from 0x40 + a 6-bit pseudorandom number
 */

static const char * const redundant_string =
	"No one would have believed in the last years of the nineteenth "
	"century that this world was being watched keenly and closely by "
	"intelligences greater than man's and yet as mortal as his own; that as "
	"men busied themselves about their various concerns they were "
	"scrutinised and studied, perhaps almost as narrowly as a man with a "
	"microscope might scrutinise the transient creatures that swarm and "
	"multiply in a drop of water.  With infinite complacency men went to "
	"and fro over this globe about their little affairs, serene in their "
	"assurance of their empire over matter. It is possible that the "
	"infusoria under the microscope do the same.  No one gave a thought to "
	"the older worlds of space as sources of human danger, or thought of "
	"them only to dismiss the idea of life upon them as impossible or "
	"improbable.  It is curious to recall some of the mental habits of "
	"those departed days.  At most terrestrial men fancied there might be "
	"other men upon Mars, perhaps inferior to themselves and ready to "
	"welcome a missionary enterprise. Yet across the gulf of space, minds "
	"that are to our minds as ours are to those of the beasts that perish, "
	"intellects vast and cool and unsympathetic, regarded this earth with "
	"envious eyes, and slowly and surely drew their plans against us.  And "
	"early in the twentieth century came the great disillusionment. "
;

/* this reflects the length of the string above */
#define REPEAT_STRING_LEN 1337
/* this is the total size of the ws message we will send */
#define MESSAGE_SIZE (100 * REPEAT_STRING_LEN)
/* this is how much we will send each time the connection is writable */
#define MESSAGE_CHUNK_SIZE (1 * 1024)

/* one of these is created for each client connecting to us */

static struct _server_vhost_data *vhost_data;


static uint64_t rng(uint64_t *r)
{
	*r ^= *r << 21;
	*r ^= *r >> 35;
	*r ^= *r << 4;

	return *r;
}

static bool header_check(void *d, size_t len)
{
	uint16_t header;
	uint8_t *pos = (uint8_t *)d;

	pkg_pull_word(header, pos);
	header = htons(header);
	log_trace("header:0x%x", header);
	
	return (header == MC_HEADER);
}

static void message_process(void *d, size_t len)
{
	char string[32] = {'\0'};
	uint8_t type, length, *pos = (uint8_t *)d;
		
	if (!header_check(d, len)) {
		log_err("header invaild");
		return;
	}

	pos += 2;
	pkg_pull_byte(type, pos);
	log_trace("type:%d", type);

	switch (type) {

	case mc_handshark:
		log_info("message:handshark");
		server_handshark_process();
		break;

	case mc_heartbeat:
		log_info("message:heartbeat");
		break;

	case mc_voicemesg:
		log_info("message:voicemesg");
		break;

	default:
		break;
	}

	return et_ok;
}

static int
callback_micro_chat(struct lws *wsi, enum lws_callback_reasons reason,
			  void *user, void *in, size_t len)
{
    struct _server_vhost_data *vhd = (struct _server_vhost_data *)
                    lws_protocol_vh_priv_get(lws_get_vhost(wsi),
                            lws_get_protocol(wsi));
	uint8_t buf[LWS_PRE + MESSAGE_SIZE], *start = &buf[LWS_PRE], *p;
	int n, m, flags, olen, amount;
	const struct msg *pmsg;
	
	switch (reason) {
        case LWS_CALLBACK_PROTOCOL_INIT:
			log_trace("PROTOCOL_INIT");
            vhd = lws_protocol_vh_priv_zalloc(lws_get_vhost(wsi),
            	lws_get_protocol(wsi),
                sizeof(struct _server_vhost_data));
			vhd->lwsctx = lws_get_context(wsi);
			vhd->protocol = lws_get_protocol(wsi);
			vhd->lwsvhost = lws_get_vhost(wsi);
			vhd->ring = lws_ring_create(sizeof(struct msg), 8,
					    destroy_message);
            if (!vhd) {log_err("alloc vhd error");return -1;}
			pthread_mutex_init(&vhd->lock_ring, NULL);
			vhost_data = vhd;
            break;

	case LWS_CALLBACK_ESTABLISHED:
		log_trace("ESTABLISHED");
		vhd->established = 1;
		break;

	case LWS_CALLBACK_SERVER_WRITEABLE:
		log_trace("SERVER_WRITEABLE");
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

	/*
	case LWS_CALLBACK_RECEIVE:
		log_trace("RECEIVE remain:%d final:%d", 
			(int)lws_remaining_packet_payload(wsi),
			lws_is_final_fragment(wsi));
		vhd->lwswsi = wsi;
		olen = (int)len;
		awoke_hexdump_trace(in, len);
		message_process(in, olen);
		break;*/

	case LWS_CALLBACK_EVENT_WAIT_CANCELLED:
		log_trace("EVENT_WAIT_CANCELLED");
		if (vhd && vhd->established)
			lws_callback_on_writable(wsi);
		log_trace("EVENT_WAIT_CANCELLED");

	default:
		break;
	}

	return 0;
}

#define LWS_PLUGIN_PROTOCOL_MICRO_CHAT \
	{ \
		"micro-chat", \
		callback_micro_chat, \
		0, \
		4096, \
		0, NULL, 0 \
	}

