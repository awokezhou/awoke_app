
#ifndef __AWOKE_QUEUE_H__
#define __AWOKE_QUEUE_H__

#include <string.h>

#include "awoke_type.h"
#include "awoke_error.h"
#include "awoke_macros.h"
#include "awoke_memory.h"

typedef struct _awoke_queue {		/* FIFO standard queue */
	int curr;
	size_t node_size;
	int node_nr;
#define AWOKE_QUEUE_F_RB	0x0001			/* queue flag rollback */
#define AWOKE_QUEUE_F_IN    0x0002          /* queue flag insert */
	uint16_t flag;
	char *_queue;
} awoke_queue;


#define awoke_queue_foreach(q, u, type)							\
	int __k;													\
	u = (type *)((q)->_queue + ((q)->curr)*((q)->node_size));	\
																\
	for (__k = (q)->curr;										\
		(__k>=0);												\
		__k--,													\
		u = (type *)((q)->_queue + (__k)*((q)->node_size))) 	\


int awoke_queue_size(awoke_queue *q);

bool awoke_queue_full(awoke_queue *q);
bool awoke_queue_empty(awoke_queue *q);
void awoke_queue_free(awoke_queue **q);
void awoke_queue_clean(awoke_queue *q);

err_type awoke_queue_deq(awoke_queue *q, void *u);
err_type awoke_queue_enq(awoke_queue *q, void *u);
err_type awoke_queue_get(awoke_queue *q, int index, void *u);
err_type awoke_queue_last(awoke_queue *q, void *u);
err_type awoke_queue_first(awoke_queue *q, void *u);
err_type awoke_queue_remove(awoke_queue *q, int index);
err_type awoke_queue_insert_after(awoke_queue *q, int index, void *u);

awoke_queue *awoke_queue_create(size_t node_size, int node_nr, uint16_t flag);



/* -- Namespace --{*/
#ifdef namespace_awoke
#ifdef namespace_queue
typedef struct _queue_namespace {
	int (*size)(awoke_queue *q);
	bool (*full)(awoke_queue *q);
	bool (*empty)(awoke_queue *q);
	void (*free)(awoke_queue **q);
	void (*clean)(awoke_queue *q);
	err_type (*get)(awoke_queue *q, int index, void *u);
	err_type (*last)(awoke_queue *q, void *u);
	err_type (*first)(awoke_queue *q, void *u);
	err_type (*remove)(awoke_queue *q, int index);
	err_type (*dequeue)(awoke_queue *q, void *u);
	err_type (*enqueue)(awoke_queue *q, void *u);
	err_type (*insert_after)(awoke_queue *q, int index, void *u);
    awoke_queue *(*create)(size_t node_size, int node_nr, uint16_t flag);    
} queue_namespace;
extern queue_namespace const Queue;
#endif
#endif


#endif /* __AWOKE_QUEUE_H__ */