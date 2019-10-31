
#ifndef __BENCHMARK_H__
#define __BENCHMARK_H__

#include "awoke_type.h"
#include "awoke_error.h"
#include "awoke_waitev.h"
#include "awoke_memory.h"
#include "condition_action.h"

typedef enum {
	arg_none = 0,
	arg_waitev_test,
	arg_condaction_test,
} benchmark_args;

typedef err_type (*bencmark_func)(void *);

#endif /* __BENCHMARK_H__ */
