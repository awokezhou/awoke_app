#ifndef __AWOKE_WORKER_H__
#define __AWOKE_WORKER_H__

#include<unistd.h>
#include <pthread.h> 

#include "awoke_type.h"
#include "awoke_list.h"
#include "awoke_event.h"
#include "awoke_error.h"
#include "awoke_memory.h"
#include "awoke_macros.h"
#include "awoke_string.h"

#define WORKER_CREATE_JOINABLE	PTHREAD_CREATE_JOINABLE

#define worker_id pthread_t
#define worker_attr pthread_attr_t
#define worker_mutex  pthread_mutex_t
#define worker_cond  pthread_cond_t
#define _worker_mutex_init(mutex, val) pthread_mutex_init(mutex, val)
#define worker_mutex_lock(mutex) pthread_mutex_lock(mutex)
#define worker_mutex_unlock(mutex) pthread_mutex_unlock(mutex)
#define worker_mutex_destroy(mutex) pthread_mutex_destroy(mutex)
#define _worker_cond_init(cond, val) pthread_cond_init(cond, val)
#define worker_condition_wait(cond, mutex) pthread_cond_wait(cond, mutex)
#define worker_condition_notify(cond) pthread_cond_signal(cond)
#define worker_condition_destroy(cond) pthread_cond_destroy(cond)

#define worker_attr_init(attr) 	pthread_attr_init(attr)
#define worker_attr_setdetachstate(attr, stat) 		pthread_attr_setdetachstate(attr, stat)
#define worker_create(id, attr, handler, context) 	pthread_create(id, attr, handler, context)
#define worker_join(tid, retstat) pthread_join(tid, retstat)
#define worker_attr_destroy(attr) pthread_attr_destroy(attr)

typedef struct _awoke_worker {

	char *name;
	worker_id wid;
	uint32_t tick;
	
#define WORKER_FEAT_PERIODICITY		0x0001
#define WORKER_FEAT_SUSPEND			0x0002
#define WORKER_FEAT_PIPE_CHANNEL	0x0004
#define WORKER_FEAT_TICK_SEC		0x0010
#define WORKER_FEAT_TICK_MSEC		0x0020
#define WORKER_FEAT_TICK_USEC		0x0040
#define WORKER_FEAT_REFERENCE		0x0080
#define WORKER_FEAT_CUSTOM_DEFINE	0x0100
	uint16_t features;

	bool _running;
	bool _force_stop;

	uint32_t _reference;
	
	worker_mutex mutex;
	worker_cond cond;
	worker_attr attr;
	
	err_type (*handler)();
	
	void *data;

	awoke_event_loop *evl;
	
	awoke_event_pipech pipe_channel;	
	
	awoke_list _head;
} awoke_worker;

typedef struct _awoke_worker_context {
	struct _awoke_worker *worker;
}awoke_worker_context;

typedef struct _awoke_worker_pipemsg {
	uint16_t src;
#define WORKER_PIPE_MSG_DST_MAIN	0
	uint16_t dst;
	uint32_t type;
	uint16_t f_event,
			 f_request,
			 f_response,
			 f_broadcast;
	uint32_t word;
	uint32_t data_len;
} awoke_worker_pipemsg;

typedef struct _awoke_tmwkr {
	int task_nr;
	awoke_worker *scheduler;
	awoke_list task_queue;
	err_type (*sched_end)(struct _awoke_tmwkr *);
	void *data;
} awoke_tmwkr;

typedef struct _awoke_tmtsk {
	char *name;
	uint32_t tid;
	uint32_t tick;
	uint32_t clock;
	bool periodic;
	bool (*can_sched)(struct _awoke_tmtsk *, struct _awoke_tmwkr *);
	err_type (*handle)(struct _awoke_tmtsk *, struct _awoke_tmwkr *);
	void (*err_proc)(struct _awoke_tmtsk *, struct _awoke_tmwkr *);
	awoke_list _head;
} awoke_tmtsk;

void awoke_worker_start(awoke_worker *wk);
void awoke_worker_stop(awoke_worker *wk);
void awoke_worker_suspend(awoke_worker *wk);
void awoke_worker_resume(awoke_worker *wk);
void awoke_worker_destroy(awoke_worker *wk);
bool awoke_worker_should_stop(awoke_worker *wk);
void awoke_worker_should_suspend(awoke_worker *wk);
awoke_worker *awoke_worker_create(char *name, uint32_t tick, 
		uint16_t features, err_type (*handler)(void *ctx), void *data);

void awoke_tmwkr_stop(awoke_tmwkr *twk);
void awoke_tmwkr_start(awoke_tmwkr *twk);
awoke_tmwkr *awoke_tmwkr_create(char *name, uint32_t clock, uint16_t features, 
									   err_type (*handler)(void *), 
									   err_type (*sched_end)(struct _awoke_tmwkr *), 
									   void *data);
err_type awoke_tmwkr_task_register(char *name, uint32_t clock, bool periodic,  
				bool (*can_sched)(struct _awoke_tmtsk *, struct _awoke_tmwkr *),
				err_type (*handle)(struct _awoke_tmtsk *, struct _awoke_tmwkr *), 
				awoke_tmwkr *twk);
void awoke_tmwkr_clean(awoke_tmwkr **twk);
void awoke_tmtsk_clean(awoke_tmtsk **tsk);

#endif /* __AWOKE_WORKER_H__ */