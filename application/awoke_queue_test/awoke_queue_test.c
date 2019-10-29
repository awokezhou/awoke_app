
#include "awoke_log.h"
#include "awoke_queue.h"

typedef struct _test_unit {
	int a;
} test_unit;

static void test_queue_dump(awoke_queue *q)
{
	int i;
	int max;
	int len;
	int size;
	test_unit *unit;
	size = q->curr+1;
	char *pos;
	char dump[256] = {'\0'};
	
	if (!size)
		return;

	len = 0;	
	max = 256;
	pos = dump;

	
	awoke_queue_foreach(q, unit, test_unit) {
		len = snprintf(pos, max, "%d ", unit->a);
		pos += len;
		max -= len;
	}

	/*
	for (i=0; i<size; i++) {
		char *n = q->_queue + i*q->node_size;
		memcpy(&unit, n, q->node_size);
		len = snprintf(pos, max, "%d ", unit.a);
		pos += len;
		max -= len;
	}*/
	
	log_debug("queue size:%d, data: [%s]", size, dump);
	printf("\n");
}

void normal_queue()
{
	awoke_queue *queue;
	test_unit u1, u2, u3, u4, u;

	queue = awoke_queue_create(sizeof(test_unit), 5, AWOKE_QUEUE_F_RB);
	if (!queue) {
		log_err("queue create error");
		return -1;
	}
	log_debug("queue create ok, q:0x%x, q->_queue:0x%x", queue, queue->_queue);

	u1.a = 1;
	u2.a = 2;
	u3.a = 3;
	u4.a = 4;

	awoke_queue_enq(queue, &u1);
	awoke_queue_enq(queue, &u2);
	awoke_queue_enq(queue, &u3);
	awoke_queue_enq(queue, &u4);

	test_queue_dump(queue);
	
	awoke_queue_deq(queue, &u);
	awoke_queue_deq(queue, &u);
	awoke_queue_deq(queue, &u);
	awoke_queue_deq(queue, &u);

	test_queue_dump(queue);

	awoke_queue_enq(queue, &u1);
	awoke_queue_enq(queue, &u2);
	awoke_queue_enq(queue, &u3);
	awoke_queue_enq(queue, &u4);

	test_queue_dump(queue);

	awoke_queue_enq(queue, &u1);
	awoke_queue_enq(queue, &u2);
	awoke_queue_enq(queue, &u3);
	awoke_queue_enq(queue, &u4);
	
	test_queue_dump(queue);
	
	awoke_queue_clean(&queue);
}

int main(int argc, char *argv[])
{
	log_mode(LOG_TEST);
	log_level(LOG_DBG);

	normal_queue();

	return 0;
}
