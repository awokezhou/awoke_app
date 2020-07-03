#define namespace_awoke
#define namespace_queue

#include "awoke_queue.h"
#include "awoke_log.h"

bool awoke_queue_empty(awoke_queue *q)
{
	return ((q->curr+1) == 0);
}

bool awoke_queue_full(awoke_queue *q)
{
	return ((q->curr+1) == q->node_nr);
}

int awoke_queue_size(awoke_queue *q)
{
	return (q->curr+1);
}

err_type awoke_queue_delete(awoke_queue *q, int index)
{
    int i;
    char *addr;

    if (q->curr < index) {
        return et_full;
    }

    if (awoke_queue_empty(q)) {
        return et_empty;
    }

    addr = q->_queue + (index)*q->node_size;
    memset(addr, 0x0, q->node_size);

    for (i=index; i<=q->curr; i++) {
        void *addr_n1 = q->_queue + i*q->node_size;
        void *addr_n2 = q->_queue + (i+1)*q->node_size;
        memcpy(addr_n1, addr_n2, q->node_size);
    }

    q->curr--;

    return et_ok;
}

err_type awoke_queue_insert_after(awoke_queue *q, int index, void *u)
{
    int i, idx;
    char *addr;

    if (!mask_exst(q->flag, AWOKE_QUEUE_F_IN)) {
        return et_param;
    }

    if (q->curr < index) {
        return et_full;
    }

	if (awoke_queue_full(q)) {
		if (mask_exst(q->flag, AWOKE_QUEUE_F_RB)) {
			void *_front = mem_alloc_z(q->node_size);
			awoke_queue_deq(q, _front);
			mem_free(_front);
			_front = NULL;
            idx = index;
		} else {
			return et_full;
		}
	} else {
        idx = index + 1;
    }

    for (i=q->curr; i>(index-1); i--) {
        void *addr_n1 = q->_queue + i*q->node_size;
        void *addr_n2 = q->_queue + (i+1)*q->node_size;
        memcpy(addr_n2, addr_n1, q->node_size);
    }

    addr = q->_queue + (idx)*q->node_size;
    memcpy(addr, u, q->node_size);

    q->curr = (q->curr+1)%q->node_nr;
    return et_ok;
}

err_type awoke_queue_get(awoke_queue *q, int index, void *u)
{
    char *addr;

	if (awoke_queue_empty(q)) {
		memset(u, 0x0, q->node_size);
		return et_empty;
	}

    addr = q->_queue + index*q->node_size;
    memcpy(u, addr, q->node_size);
    return et_ok;
}

err_type awoke_queue_last(awoke_queue *q, void *u)
{
    char *addr;

	if (awoke_queue_empty(q)) {
		memset(u, 0x0, q->node_size);
		return et_empty;
	}

    addr = q->_queue + q->curr*q->node_size;
    memcpy(u, addr, q->node_size);
    return et_ok;
}

err_type awoke_queue_deq(awoke_queue *q, void *u)
{
	int i;
	char *addr;
		
	if (awoke_queue_empty(q)) {
		memset(u, 0x0, q->node_size);
		return et_empty;
	}

	addr = q->_queue;
	memcpy(u, addr, q->node_size);
	
	for (i=0; i<q->curr; i++) {
		void *addr_n1 = q->_queue + i*q->node_size;
		void *addr_n2 = q->_queue + (i+1)*q->node_size;
		memcpy(addr_n1, addr_n2, q->node_size);
		//memcpy(&q->_queue[i], &q->_queue[i+1], q->node_size);
	}

	addr = q->_queue + q->curr*q->node_size;
	memset(addr, 0x0, q->node_size);
	
	q->curr--;
	
	return et_ok;
}

err_type awoke_queue_enq(awoke_queue *q, void *u)
{
	char *addr;
	
	if (awoke_queue_full(q)) {
		if (mask_exst(q->flag, AWOKE_QUEUE_F_RB)) {
			void *_front = mem_alloc_z(q->node_size);
			awoke_queue_deq(q, _front);
			mem_free(_front);
			_front = NULL;
		} else {
			return et_full;
		}
	}

	q->curr = (q->curr+1)%q->node_nr;
	addr = q->_queue + q->curr*q->node_size;
	memcpy(addr, u, q->node_size);
	//memcpy(&q->_queue[q->curr], u, q->node_size);
	return et_ok;
}

awoke_queue *awoke_queue_create(size_t node_size, int node_nr, uint16_t flag)
{
	awoke_queue *q;
	int alloc_size;
	
	alloc_size = sizeof(awoke_queue) + node_size*node_nr;

	q = mem_alloc_z(alloc_size);
	if (!q) {
		return NULL;
	}
	memset(q, 0x0, alloc_size);

	q->curr = -1;
	q->node_size = node_size;
	q->node_nr = node_nr;
	q->_queue = (char *)q + sizeof(awoke_queue);
	mask_push(q->flag, flag);
	return q;
}

void awoke_queue_clean(awoke_queue **q)
{
	int i;
	int size;
	awoke_queue *p;
	void *_front = NULL;

	if (!q || !*q)
		return;

	p = *q;

	_front = mem_alloc_z(p->node_size);

	size = awoke_queue_size(p);
	if (!size)
		return;

	for (i=0; i<size; i++) {
		awoke_queue_deq(p, _front);
	}

	mem_free(_front);
	_front = NULL;

	mem_free(p);
	p = NULL;
	return;
}

namespace const Queue = {
    .create = awoke_queue_create,
    .empty = awoke_queue_empty,
    .full = awoke_queue_full,
    .get = awoke_queue_get,
    .last = awoke_queue_last,
    .length = awoke_queue_size,
    .dequeue = awoke_queue_deq,
    .enqueue = awoke_queue_enq,
    .delete = awoke_queue_delete,
    .insert_after = awoke_queue_insert_after,
};