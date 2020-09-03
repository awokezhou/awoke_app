
#ifndef __CLIENT_H__
#define __CLIENT_H__



#include <libwebsockets.h>
#include <pthread.h>

#include "awoke_type.h"
#include "awoke_error.h"


typedef enum {
	CLIENT_STATE_NONE = 0,
	CLIENT_STATE_HANDSHARK,
	CLIENT_STATE_CONNECTED,
} client_state;
	

typedef struct _client_vhost_data {
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
	client_state state;
} client_vhost_data;



void destroy_message(void *_msg);

#define client_log(level, ...)		awoke_logm(level, 	LOG_M_MC_CLIENT, __func__, __LINE__, __VA_ARGS__)

#define client_burst(...)	client_log(LOG_BURST, __VA_ARGS__);
#define client_trace(...)	client_log(LOG_TRACE, 	__VA_ARGS__)
#define client_debug(...)	awoke_logm(LOG_DBG,	  	LOG_M_MC_CLIENT, __func__, __LINE__, __VA_ARGS__)
#define client_info(...)	awoke_logm(LOG_INFO,	LOG_M_MC_CLIENT, __func__, __LINE__, __VA_ARGS__)
#define client_warn(...)	awoke_logm(LOG_WARN,	LOG_M_MC_CLIENT, __func__, __LINE__, __VA_ARGS__)
#define client_err(...)		awoke_logm(LOG_ERR,		LOG_M_MC_CLIENT, __func__, __LINE__, __VA_ARGS__)
#define client_bug(...)		awoke_logm(LOG_BUG,		LOG_M_MC_CLIENT, __func__, __LINE__, __VA_ARGS__)


#endif /* __CLIENT_H__ */

