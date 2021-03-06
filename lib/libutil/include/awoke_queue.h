
#ifndef __AWOKE_QUEUE_H__
#define __AWOKE_QUEUE_H__

#include <string.h>

#include "awoke_type.h"
#include "awoke_error.h"
#include "awoke_macros.h"
#include "awoke_memory.h"



/* -- dump struct --{*/
typedef struct _awoke_queue_dumpinfo {
	uint8_t width;
	void (*value_dump)(void *s, int width, char *buff, int len);
	void (*priro_dump)(void *s, int width, char *buff, int len);
} awoke_queue_dumpinfo;
/*}-- dump struct -- */


/* -- Queue -- {*/
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
/*}-- Queue -- */


/* -- MinPQ --{*/
typedef struct _awoke_minpq {
    int node_nr;
    int capacity;
    size_t nodesize;
    int *p;
    char *q;
#define AWOKE_MINPQ_F_CDC   0x0001  /* Custom define comparator */
#define AWOKE_MINPQ_F_RES   0x0002  /* Resize */
    uint16_t flags;
    bool (*comparator)(void *, void *);
	awoke_queue_dumpinfo dumpinfo;
} awoke_minpq;

int awoke_minpq_size(awoke_minpq *q);
bool awoke_minpq_full(struct _awoke_minpq *q);
bool awoke_minpq_empty(struct _awoke_minpq *q);
void awoke_minpq_dumpinfo_set(struct _awoke_minpq *q, int width,
    void (*value_dump)(void *, int, char *, int),
    void (*prior_dump)(void *, int, char *, int));
err_type minpq_min(struct _awoke_minpq *q, int *p);
err_type awoke_minpq_insert(struct _awoke_minpq *q, void *u, int p);
err_type awoke_minpq_delmin(struct _awoke_minpq *q, void *u, int *p);
awoke_minpq *awoke_minpq_create(size_t node_size, int capacity, 
    bool(*comparator)(void *, void *), uint16_t flags);
void awoke_minpq_free(struct _awoke_minpq **q);
err_type awoke_minpq_get(struct _awoke_minpq *q, void *u, int *p, int index);
err_type awoke_minpq_del(struct _awoke_minpq *q, void *u, int *p, int index);
err_type awoke_minpq_init(struct _awoke_minpq *q, size_t nodesize, int capacity, 
    bool(*comparator)(void *, void *), uint16_t flags);
/*}-- MinPQ -- */


/* -- FIFO --{*/
typedef struct _awoke_fifo {
	char *q;
#define AWOKE_FIFO_F_CDF   0x0001  /* custom define free */
#define AWOKE_FIFO_F_RES   0x0002  /* Resize */
#define AWOKE_FIFO_F_RBK   0x0004  /* Rollback */
	uint16_t flags;
	size_t nodesize;
	unsigned int head;
	unsigned int tail;
	unsigned int node_nr;
	unsigned int capacity;
	awoke_queue_dumpinfo dumpinfo;
} awoke_fifo;

err_type awoke_fifo_get(struct _awoke_fifo *f, void *u, int index);
bool awoke_fifo_empty(struct _awoke_fifo *f);
bool awoke_fifo_full(struct _awoke_fifo *f);
err_type awoke_fifo_resize(struct _awoke_fifo *f, unsigned int capacity);
err_type awoke_fifo_size(struct _awoke_fifo *f);
err_type awoke_fifo_dequeue(struct _awoke_fifo *f, void *u);
err_type awoke_fifo_enqueue(struct _awoke_fifo *f, void *u);
err_type awoke_fifo_init(struct _awoke_fifo *f, void *addr, int nodesize, int capacity, uint16_t flags);
awoke_fifo *awoke_fifo_create(size_t nodesize, int capacity, uint16_t flags);
void awoke_fifo_dumpinfo_set(struct _awoke_fifo *f, int width,
    void (*value_dump)(void *, int, char *, int),
    void (*prior_dump)(void *, int, char *, int));
void awoke_fifo_dump(struct _awoke_fifo *f);
/*}-- FIFO -- */


/*
 * Array FIFO {
 */
void awoke_afifo_create(void *addr, int nodesize, int capacity, struct _awoke_fifo *f);
err_type awoke_afifo_enqueue(struct _awoke_fifo *f, void *u);
err_type awoke_afifo_dequeue(struct _awoke_fifo *f, void *u);
/*
 * } Array FIFO
 */


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

typedef struct _minpq_namespace {
    int (*size)(awoke_minpq *q);
    bool (*full)(awoke_minpq *q);
    bool (*empty)(awoke_minpq *q);
    err_type (*min)(awoke_minpq *q, int *p);
    err_type (*insert)(awoke_minpq *q, void *u, int p);
    err_type (*delmin)(awoke_minpq *q, void *u, int *p);
    awoke_minpq *(*create)(size_t node_size, int capacity, 
        bool(*comparator)(void *, void *), uint16_t flags);
    void (*info)(awoke_minpq *q);
    err_type (*get)(struct _awoke_minpq *q, void *u, int *p, int index);
	err_type (*del)(struct _awoke_minpq *q, void *u, int *p, int index);
    void (*set_info_handle)(struct _awoke_minpq *q, int,
        void (*info_prior_handle)(struct _awoke_minpq *, int, char *, int),
        void (*info_value_handle)(struct _awoke_minpq *, int, char *, int));
} minpq_namespace;
extern minpq_namespace const MinPQ;
#endif
#endif


#endif /* __AWOKE_QUEUE_H__ */