
#include "cc_deque.h"

bool cc_deque_full(cc_deque *q)
{
	return ((q->rear+1-q->front) == q->size);
}

bool cc_deque_empty()
{
	
}

cc_deque *cc_deque_create(int size) {
	cc_deque *q;

	q = mem_alloc_z(sizeof(cc_deque));
	if (!q)
		return NULL;

	q->_queue = mem_alloc_z(sizeof(int)*size);
	if (!q->_queue) {
		mem_free(q);
		return NULL;
	}

	q->front = 0;
	q->rear = 0;
	q->size = size;
	return q;
}

err_type cc_deque_insert_front(cc_deque *q, int value)
{
	if (cc_deque_full(q)) return et_fail;

	q->front = (q->front==0) ? (q->size-1) : (q->front-1);
	q->_queue[q->front] = value;
	return et_ok;
}

err_type cc_deque_insert_last(cc_deque *q, int value)
{
	if (cc_deque_full(q)) return et_fail;

	q->rear = (q->rear+1)%(q->size);
	q->_queue[q->rear] = value;
	return et_ok;
}

err_type cc_deque_delete_front(cc_deque *q)
{
	
}