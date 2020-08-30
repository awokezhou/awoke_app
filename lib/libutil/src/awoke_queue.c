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

err_type awoke_queue_first(awoke_queue *q, void *u)
{
	char *addr; 
	
	if (awoke_queue_empty(q)) {
		memset(u, 0x0, q->node_size);
		return et_empty;
	}

	addr = q->_queue;
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

void awoke_queue_clean(awoke_queue *q)
{
	int i;
	int size;
	void *_front = mem_alloc(q->node_size);

	size = awoke_queue_size(q);
	if (!size)
		return;

	for (i=0; i<size; i++) {
		awoke_queue_deq(q, _front);
	}

	mem_free(_front);
	_front = NULL;
}

void hqnb_queue_free(awoke_queue **q)
{
	awoke_queue *p;

	if (!q || !*q)
		return;

	p = *q;

	mem_free(p);
	p = NULL;
	return;
}

queue_namespace const Queue = {
	.size = awoke_queue_size,
	.full = awoke_queue_full,
    .empty = awoke_queue_empty,
    .get = awoke_queue_get,
    .last = awoke_queue_last,
    .first = awoke_queue_first,
    .dequeue = awoke_queue_deq,
    .enqueue = awoke_queue_enq,
    .remove = awoke_queue_delete,
    .insert_after = awoke_queue_insert_after,
    .create = awoke_queue_create,
};

int awoke_minpq_size(struct _awoke_minpq *q)
{
    return (q->node_nr);
}

bool awoke_minpq_full(struct _awoke_minpq *q)
{
    return (q->capacity == q->node_nr);
}

bool awoke_minpq_empty(struct _awoke_minpq *q)
{
    return (q->node_nr == 0);
}

err_type awoke_minpq_resize(struct _awoke_minpq *q, int capcaity)
{
    int i;
    char *new;
    
    if (capcaity <= q->node_nr) {
        log_err("capcaity invaild");
        return et_param;
    }

    new = mem_alloc_z(q->nodesize*(capcaity+1));
    if (!new) {
        log_err("alloc new error");
        return et_nomem;
    }

    memcpy(new, q->q, q->nodesize*(q->node_nr+1));
    mem_free(q->q);
    q->q = new;
    q->capacity = capcaity;
    return et_ok;
}

err_type awoke_minpq_min(struct _awoke_minpq *q, int *p)
{
    if (awoke_minpq_empty(q)) {
        log_err("MinPQ empty");
        return et_empty;
    }

    *p = q->p[1];
    return et_ok;
}

static bool minpq_greater(struct _awoke_minpq *q, int i, int j)
{
    char *addr1, *addr2;

    if (!q->comparator) {
        return (q->p[i] > q->p[j]);
    } else {
        addr1 = q->q + q->nodesize*i;
        addr2 = q->q + q->nodesize*j;
        return q->comparator(addr1, addr2);
    }
}

static void minpq_swap(struct _awoke_minpq *q, int i, int j)
{
    int tempp;
    char *addr1, *addr2;
    char *tempu = mem_alloc_z(q->nodesize);

    tempp = q->p[i];
    q->p[i] = q->p[j];
    q->p[j] = tempp;

    addr1 = q->q + q->nodesize*i;
    addr2 = q->q + q->nodesize*j;
    memcpy(tempu, addr1, q->nodesize);
    memcpy(addr1, addr2, q->nodesize);
    memcpy(addr2, tempu, q->nodesize);
}

static void minpq_sink(struct _awoke_minpq *q, int k)
{
    int j;

    while (2*k <= q->node_nr) {
        j = 2*k;
        if ((j<q->node_nr) && (minpq_greater(q, j, j+1)))
            j += 1;
        if (!minpq_greater(q, k, j)) 
            break;
        minpq_swap(q, k, j);
        k = j;
    }
}

static void minpq_swim(struct _awoke_minpq *q, int k)
{
    while ((k>1) && (minpq_greater(q, k/2, k))) {
        minpq_swap(q, k, k/2);
        //log_debug("swap %d %d", k, k/2);
        k = k/2;
    }
}

err_type awoke_minpq_get(struct _awoke_minpq *q, void *u, int *p, int index)
{
    char *addr;

    if (awoke_minpq_empty(q)) {
        log_err("MinPQ empty");
        return et_empty;
    }

    addr = q->q + q->nodesize*index;
    memcpy(u, addr, q->nodesize);
    *p = q->p[index];
    return et_ok;
}

err_type awoke_minpq_insert(struct _awoke_minpq *q, void *u, int p)
{
    char *addr;

    if (awoke_minpq_full(q)) {
        if (mask_exst(q->flags, AWOKE_MINPQ_F_RES)) {
            if (!q->capacity) q->capacity = 1;
            awoke_minpq_resize(q, q->capacity*2);
        } else {
            log_err("MinPQ full");
            return et_full;
        }
    }

    q->p[++q->node_nr] = p;
    addr = q->q + q->nodesize*q->node_nr;
    memcpy(addr, u, q->nodesize);
    minpq_swim(q, q->node_nr);
    return et_ok;
}

err_type awoke_minpq_delmin(struct _awoke_minpq *q, void *u, int *p)
{
    char *addr1, *addr2;

    if (awoke_minpq_empty(q)) {
        log_err("MinPQ empty");
        return et_empty;
    }

    addr1 = q->q + q->nodesize;
    memcpy(u, addr1, q->nodesize);
    *p = q->p[1];
    q->p[1] = q->p[q->node_nr];
    addr2 = q->q + q->nodesize*q->node_nr;
    memcpy(addr1, addr2, q->nodesize);
    q->node_nr--;
    minpq_sink(q, 1);
    q->p[q->node_nr+1] = 0;

    if ((q->node_nr>0) && 
        (q->node_nr==(q->capacity-1)/4) &&
        mask_exst(q->flags, AWOKE_MINPQ_F_RES)) {
        awoke_minpq_resize(q, q->capacity/2);
    }

    addr2 = q->q + q->nodesize*(q->node_nr+1);
    memset(addr2, 0x0, q->nodesize);
    return et_ok;
}

awoke_minpq *awoke_minpq_create(size_t nodesize, int capacity, 
    bool(*comparator)(void *, void *), uint16_t flags)
{
    int allocnr;

    if (nodesize <= 0) {log_err("nodesize zero");return NULL;}
    if (capacity < 0) {log_err("capacity invaild");return NULL;}

    awoke_minpq *q = mem_alloc_z(sizeof(awoke_minpq));
    if (!q) {log_err("MinPQ create error");return NULL;}

    q->node_nr = 0;
    q->nodesize = nodesize;
    q->info_width = 4;

    q->capacity = capacity;
    if (capacity == 0) {
        mask_push(q->flags, AWOKE_MINPQ_F_RES);
    }

    if (comparator != NULL) {
        q->comparator = comparator;
        mask_push(q->flags, AWOKE_MINPQ_F_CDC);
    }
    
    mask_push(q->flags, flags);

    if (!capacity) {
        allocnr = 1;
    } else {
        allocnr = capacity + 1;
    }

    q->p = mem_alloc_z(allocnr*sizeof(int));
    q->q = mem_alloc_z(allocnr*q->node_nr);

    return q;
}

void awoke_minpq_info_set_handle(struct _awoke_minpq *q, int width,
    void (*info_prior_handle)(struct _awoke_minpq *, int, char *, int),
    void (*info_value_handle)(struct _awoke_minpq *, int, char *, int))
{
    q->info_width = width;
    q->info_prior_handle = info_prior_handle;
    q->info_value_handle = info_value_handle;
}

static void minpq_build_infodump_line(char *buff, int len, int line_nr)
{
    int i;
    build_ptr bp = build_ptr_init(buff, len);

    for (i=0; i<line_nr; i++) {
        build_ptr_string(bp, "-");
    }
}

void awoke_minpq_info_dump(struct _awoke_minpq *q)
{
    int i;
    int line_nr;
    char buff[256] = {'\0'};

    build_ptr bp = build_ptr_init(buff, 256);

    /* calculate line number */
    line_nr = (q->capacity+1)*q->info_width + 6;

    /* dump header */
    log_debug("");
    log_debug("-- MinPQ dump --");
    log_debug("capacity:%d", q->capacity);
    log_debug("size:%d", q->node_nr);
    log_debug("flag:0x%x", q->flags);

    /* dump line */
    minpq_build_infodump_line(buff, 256, line_nr);
    log_debug("%s", buff);
    memset(buff, 0x0, 256);

    /* dump index */
    build_ptr_string(bp, "index:");
    for (i=0; i<(q->capacity+1); i++) {
        build_ptr_format(bp, "%*d", q->info_width, i);
    }
    log_debug("%.*s", bp.len, bp.head);
    memset(buff, 0x0, 256);

    /* dump line */
    minpq_build_infodump_line(buff, 256, line_nr);
    log_debug("%s", buff);
    memset(buff, 0x0, 256);

    /* dump priority */
    if (mask_exst(q->flags, AWOKE_MINPQ_F_CDC) && 
        (q->info_prior_handle)) {
        q->info_prior_handle(q, q->info_width, buff, 256);
        log_debug("%s", buff);
    } else {
        bp = build_ptr_make(buff, 256);
        build_ptr_string(bp, "prior:");
        for (i=0; i<(q->node_nr+1); i++) {
            build_ptr_format(bp, "%3d", q->p[i]);
            log_debug("%.*s", bp.len, bp.head);
        }
    }
    memset(buff, 0x0, 256);
    
    /* dump line */
    minpq_build_infodump_line(buff, 256, line_nr);
    log_debug("%s", buff);
    memset(buff, 0x0, 256);

    /* dump value */
    if (q->info_value_handle) {
        q->info_value_handle(q, q->info_width, buff, 256);
        log_debug("%s", buff);
        memset(buff, 0x0, 256);
        /* dump line */
        minpq_build_infodump_line(buff, 256, line_nr);
        log_debug("%s", buff);
    }
}

minpq_namespace const MinPQ = {
    .size = awoke_minpq_size,
    .full = awoke_minpq_full,
    .empty = awoke_minpq_empty,
    .min = awoke_minpq_min,
    .insert = awoke_minpq_insert,
    .delmin = awoke_minpq_delmin,
    .create = awoke_minpq_create,
    .info = awoke_minpq_info_dump,
    .get = awoke_minpq_get,
    .set_info_handle = awoke_minpq_info_set_handle,
};