
#ifndef __SERVER_H__
#define __SERVER_H__


#include "awoke_type.h"
#include "awoke_error.h"
#include <libwebsockets.h>

#define MC_HEADER	0x8988

typedef struct _server_vhost_data {
	struct lws_context *lwsctx;
	struct lws_vhost *lwsvhost;
	struct lws *lwswsi;
	int lwsport;
	char *lwshost;
	const struct lws_protocols *protocol;
	lws_sorted_usec_list_t sul;

	pthread_mutex_t lock_ring; /* serialize access to the ring buffer */
	struct lws_ring *ring; /* ringbuffer holding unsent messages */
	uint32_t ring_tail;
	
	struct lws_client_connect_info connect;
	
	bool established;
	bool finished;
} server_vhost_data;

struct msg {
	uint8_t type;
	void *payload; /* is malloc'd */
	size_t len;
};

typedef enum {
	mc_none = 0,
	mc_handshark,
	mc_heartbeat,
	mc_voicemesg,
} mirco_chat_type;


#endif /* __SERVER_H__ */
