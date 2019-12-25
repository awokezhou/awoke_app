
#ifndef __BENCHMARK_H__
#define __BENCHMARK_H__

#include "awoke_log.h"
#include "awoke_type.h"
#include "awoke_lock.h"
#include "awoke_error.h"
#include "awoke_event.h"
#include "awoke_queue.h"
#include "awoke_waitev.h"
#include "awoke_memory.h"
#include "awoke_worker.h"
#include "awoke_string.h"
#include "awoke_http.h"

#include "vin_parser.h"
#include "condition_action.h"

typedef enum {
	arg_none = 0,
	arg_waitev_test,
	arg_condaction_test,
	arg_event_channel,
	arg_worker,
	arg_pthread,
	arg_bitflag,
	arg_lock,
	arg_timer_worker,
	arg_queue_zero_filter,
	arg_vin_parse_test,
	arg_http_request_test,
} benchmark_args;

typedef enum {
	arg_http_none = 0,
	arg_http_content_type,
	arg_http_authorization,
	arg_http_accept,
	arg_http_connection,
	arg_http_user_agent,
} http_args;

typedef err_type (*bencmark_func)(int argc, char *argv[]);



typedef struct _waitev_test_t {
	int count;
	int status;
} waitev_test_t;

typedef struct _event_channel_test_t {
	int count;
	awoke_event_loop *evl;
	awoke_event_pipech pipe_channel;
} event_channel_test_t;

typedef struct _pipech_message_header {
	uint16_t src;
	uint16_t dst;
	uint32_t msg_type;

	uint16_t f_event,
			 f_request,
			 f_response,
			 f_broadcast;

	uint32_t word;
	uint32_t data_len;

	struct _pipech_message_header *next;	
} pipech_msg_header;

typedef struct _lock_test_t {
	awoke_lock *mux_lock;
	awoke_lock *sem_lock;
	awoke_worker *wk;
} lock_test_t;


typedef struct _timer_worker_test_t {
	awoke_tmwkr *twk;
	int x;
} timer_worker_test_t;


#endif /* __BENCHMARK_H__ */
