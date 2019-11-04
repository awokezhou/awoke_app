
#ifndef __BENCHMARK_H__
#define __BENCHMARK_H__

#include "awoke_log.h"
#include "awoke_type.h"
#include "awoke_error.h"
#include "awoke_event.h"
#include "awoke_waitev.h"
#include "awoke_memory.h"
#include "awoke_worker.h"
#include "awoke_string.h"
#include "condition_action.h"

typedef enum {
	arg_none = 0,
	arg_waitev_test,
	arg_condaction_test,
	arg_event_channel,
	arg_worker,
	arg_pthread,
} benchmark_args;

typedef err_type (*bencmark_func)(void *);



typedef struct _waitev_test_t {
	int count;
	int status;
} waitev_test_t;

typedef struct _event_channel_test_t {
	int count;
	awoke_event_loop *evl;
	awoke_event_pipech pipe_channel;
} event_channel_test_t;

#endif /* __BENCHMARK_H__ */
