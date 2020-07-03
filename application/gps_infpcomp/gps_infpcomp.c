#define namespace_awoke
#define namespace_queue

#include "gps_infpcomp.h"
#include "awoke_log.h"
#include <time.h>

gps_optimizer *g_optimizer = NULL;

static gps_optimizer *get_optimizer()
{
    return g_optimizer;
}

err_type generate_point(struct _gps_optimizer *op, struct _gps_point *p)
{
    if (op->geninfp) {
        //log_debug("generate infpoint");
        p->is_infp = TRUE;
        p->data = 180;
    } else {
        //log_debug("generate norpoint");
        p->is_infp = FALSE;
        p->data = 90;
    }

    time(&p->time);
    p->history = FALSE;
    //log_debug("time:%d history:%d", p->time, p->history);

    return et_ok;
}

err_type history_update(struct _gps_history *history, struct _gps_point *p)
{
    gps_point point;
    if (!history->locked) {
        memcpy(&point, p, sizeof(gps_point));
        point.history = TRUE;
        Queue.enqueue(history->q_cache, &point);
        log_debug("history update size:%d", Queue.length(history->q_cache));
    }

    return et_ok;
}

bool history_locked(struct _gps_history *history)
{
    return history->locked;
}

void history_lock(struct _gps_history *history)
{
    if (!history_locked(history)) {
        history->locked = TRUE;
        log_debug("history lock");
    }
}

void history_unlock(struct _gps_history *history)
{
    if (history_locked(history)) {
        history->locked = FALSE;
        log_debug("history unlock");
    }
}

bool inflection_point(struct _gps_point *p)
{
    return (p->is_infp);
}

void history_delete_repeat(struct _gps_history *hi, time_t store_time)
{
    int i;
    gps_point p;

    for (i=0; i<Queue.length(hi->q_cache); i++) {
        Queue.get(hi->q_cache, i, &p);
        //log_debug("p time:%d storetime:%d", p.time, store_time);
        if (p.time == store_time) {
            log_debug("time %d repeat, delete it", store_time);
            Queue.delete(hi->q_cache, i);
            break;
        }
    }
}

void history_delete_nouse(struct _gps_history *hi)
{
    int i;
    int nouse;
    gps_point p;
    
    nouse = Queue.length(hi->q_cache) - hi->select_nr;
    log_debug("history size:%d nouse:%d select:%d", 
        Queue.length(hi->q_cache), nouse, hi->select_nr);
    for (i=0; i<nouse; i++) {
        Queue.dequeue(hi->q_cache, &p);
    }
}

void history_points_select(struct _gps_history *hi, time_t store_time, 
    int fixed_size)
{
    if ((hi->select_nr) || (hi->locked)) {
        //log_debug("history selected, no need");
        return;
    }

    if (fixed_size) {
        hi->select_nr = fixed_size;
    } else {
        //hi->select_nr = history_get_select_nr();
    }

    /* delete repeat */
    history_delete_repeat(hi, store_time);

    /* nouse dequeue */
    history_delete_nouse(hi);
}

void history_copy_one(struct _gps_history *history, awoke_queue *q)
{
    int i;
    int index = -1;
    gps_point p, see;

    Queue.dequeue(history->q_cache, &p);

    if (Queue.empty(q)) {
        Queue.enqueue(q, &p);
        return;
    }

    Queue.last(q, &see);
    //log_debug("point time:%d see time:%d", p.time, see.time);
    if (see.time == p.time) {
        /* do nothing */
    } else if (see.time < p.time) {
        //log_debug("enqueue");
        Queue.enqueue(q, &p);
    } else {
        for (i=0; i<Queue.length(q); i++) {
            Queue.get(q, i, &see);
            if (see.time > p.time) {
                index = i-1;
                break;
            }
        }
        
        if (index>0) {
            Queue.insert_after(q, index, &p);
        }
    }
}

void history_points_copy(struct _gps_history *history, awoke_queue *q)
{   
    while (!Queue.full(q) && 
           !Queue.empty(history->q_cache)) {
        history_copy_one(history, q);
    }

    /*
    log_debug("after copy, store:%d history:%d", 
        awoke_queue_size(q),
        awoke_queue_size(history->q_cache));
    */

    if ((Queue.empty(history->q_cache)) &&
        (history->select_nr != 0)) {
        history->select_nr = 0;
        log_debug("history select clean");
    }
}

bool store_allow(gps_optimizer *op)
{
    time_t now;

    time(&now);

    if ((now-op->store_time) >= 10) {
        //log_debug("time to store");
        return TRUE;
    }

    return FALSE;
}

err_type gps_parser_process(struct _awoke_worker *wk)
{
    bool should_store = FALSE;
    gps_point new_point;
    gps_optimizer *op = get_optimizer();

    generate_point(op, &new_point);

    if (new_point.is_infp) {
        log_debug("point %d inflection", new_point.time);
    } else {
        log_debug("point %d normal", new_point.time);
    }

    history_update(&op->history, &new_point);

    if (inflection_point(&new_point)) {
        history_points_select(&op->history, op->store_time, 5);
        history_points_copy(&op->history, op->q_store);
        if (!history_locked(&op->history)) {
            history_lock(&op->history);
        } else {
            should_store = TRUE;
        }
    } else {
        history_unlock(&op->history);
        if (store_allow(op)) {
            should_store = TRUE;
        }
    }

    if (should_store) {
        Queue.enqueue(op->q_store, &new_point);
        time(&op->store_time);
        log_debug("store time:%d size:%d", op->store_time,
            Queue.length(op->q_store));
    }

    return et_ok;
}

err_type poller_process(struct _awoke_worker *wk)
{   
    int i;
    int size;
    gps_point dep;
    gps_optimizer *optimizer = get_optimizer(); 

    if (Queue.full(optimizer->q_store)) {
        log_debug("store full, send");

        log_debug("\r\n-- point dump --");
        log_debug("infp\ttime\t\thistory");
        size = Queue.length(optimizer->q_store);
        for (i=0; i<size; i++) {
            Queue.dequeue(optimizer->q_store, &dep);
            log_debug("%d\t\t%d\t%d", dep.is_infp, dep.time, dep.history);
        }
    }

    return et_ok;
}

gps_optimizer *gps_optimizer_create()
{
    gps_optimizer *optimizer;

    optimizer = mem_alloc_z(sizeof(gps_optimizer));
    if (!optimizer) {
        return NULL;
    }

    optimizer->parser = awoke_worker_create(
        "gps-parser", 
        1, 
        WORKER_FEAT_PERIODICITY|WORKER_FEAT_SUSPEND,
        gps_parser_process,
        NULL);
    if (!optimizer->parser) {
        log_err("create gps parser error");
        return NULL;
    }

    optimizer->poller = awoke_worker_create(
        "poller",
        1,
        WORKER_FEAT_PERIODICITY|WORKER_FEAT_SUSPEND,
        poller_process,
        NULL
    );
    if (!optimizer->poller) {
        log_err("create poller error");
        return NULL;
    }

    optimizer->q_store = Queue.create(
        sizeof(gps_point), 10, AWOKE_QUEUE_F_RB|AWOKE_QUEUE_F_IN);
    if (!optimizer->q_store) {
        log_err("store create error");
        return NULL;
    }

    log_debug("store size:%d", Queue.length(optimizer->q_store));

    optimizer->history.q_cache = Queue.create(
        sizeof(gps_point), 20, AWOKE_QUEUE_F_RB);

    optimizer->history.locked = FALSE;

    optimizer->geninfp = FALSE;

    g_optimizer = optimizer; 

    return optimizer;
}

err_type gps_optimizer_init(struct _gps_optimizer *optimizer)
{
    log_debug("worker parser start");
    awoke_worker_start(optimizer->parser);
    log_debug("worker poller start");
    awoke_worker_start(optimizer->poller);
    return et_ok;
}

int main(int argc, char **argv)
{
    char key;
    err_type ret;
    gps_optimizer *optimizer;

    log_debug("main in");

    optimizer = gps_optimizer_create();
    if (!optimizer) {
        log_err("optimizer create error");
        return 0;
    }

    log_debug("optimizer create ok");

    ret = gps_optimizer_init(optimizer);
    if (ret != et_ok) {
        log_err("optimizer init error, ret %d", ret);
        return 0;
    }

    while (1) {
        sleep(1);
        printf("Please input key:(i/n/k)\r\n");
        key = getchar();
        //log_debug("input key:%s", key);
        if (key == 'k') {
            log_debug("kill");
            break;
        } else if (key == 'i') {
            optimizer->geninfp = TRUE;
        } else if (key == 'n') {
            optimizer->geninfp = FALSE;
        }
    }

    //awoke_worker_stop(optimizer->parser);
    //awoke_worker_destroy(optimizer->parser);

    return 0;
}