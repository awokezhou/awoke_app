
#include "awoke_worker.h"
#include "awoke_log.h"

/*
 * worker define -------------------------------- >>>>
 * 
 * -- Version --
 * 1.1.0
 */

static err_type worker_mutex_init(awoke_worker *wk)
{
	int ret;

	ret = _worker_mutex_init(&wk->mutex, NULL);

	if (ret < 0) {
		return et_worker_mutex;
	}

	return et_ok;
}

static err_type worker_condition_init(awoke_worker *wk)
{
	int ret;

	ret = _worker_cond_init(&wk->cond, NULL);

	if (ret < 0) {
		return et_worker_condition;
	}

	return et_ok;
}

static void worker_clean(awoke_worker **wk)
{
	awoke_worker *p;

	if (!wk || !*wk)
		return;

	p = *wk;

	if (p->name)
		mem_free(p->name);

	if (mask_exst(p->features, WORKER_FEAT_PIPE_CHANNEL)) {
		awoke_event_pipech_clean(p->evl, &p->pipe_channel);
		awoke_event_loop_clean(&p->evl);
	}

	mem_free(p);
	p = NULL;
}

void awoke_worker_start(awoke_worker *wk)
{
	worker_mutex_lock(&wk->mutex);
	wk->_running = TRUE;
	worker_condition_notify(&wk->cond);
	worker_mutex_unlock(&wk->mutex);
}

void awoke_worker_stop(awoke_worker *wk)
{
	worker_mutex_lock(&wk->mutex);
	wk->_force_stop = TRUE;
	log_debug("worker %s stop", wk->name);
	worker_mutex_unlock(&wk->mutex);
}

void awoke_worker_suspend(awoke_worker *wk)
{
    if (wk->_running) {  
        worker_mutex_lock(&wk->mutex);
        wk->_running = FALSE;
		worker_condition_notify(&wk->cond);
		log_debug("worker %s suspend", wk->name);
        worker_mutex_unlock(&wk->mutex);
    }
}

void awoke_worker_resume(awoke_worker *wk)
{
    if (!wk->_running) {  
        worker_mutex_lock(&wk->mutex);
        wk->_running = TRUE;
        worker_condition_notify(&wk->cond);
		log_debug("worker %s resume", wk->name);
        worker_mutex_unlock(&wk->mutex);
    }
}

void awoke_worker_destroy(awoke_worker *wk)
{
	void *ret;
	
	worker_join(wk->wid, &ret);

	worker_attr_destroy(&wk->attr);

	worker_mutex_destroy(&wk->mutex);

	worker_condition_destroy(&wk->cond);

	worker_clean(&wk);
}

static void worker_reference(awoke_worker *wk)
{
	if (mask_exst(wk->features, WORKER_FEAT_REFERENCE)) {
		wk->_reference++;
		if (wk->_reference >= 0xfffffffe) {
			wk->_reference = 0;
		}
	}
}

static bool worker_should_stop(awoke_worker *wk)
{
	return (wk->_force_stop);
}

bool awoke_worker_should_stop(awoke_worker *wk)
{
	return worker_should_stop(wk);
}

static void worker_should_suspend(awoke_worker *wk)
{
	worker_mutex_lock(&wk->mutex);
	
	while (!wk->_running) {
		log_debug("worker %s condition wait", wk->name);
		worker_condition_wait(&wk->cond, &wk->mutex);
	}
	
	worker_mutex_unlock(&wk->mutex);		
}

void awoke_worker_should_suspend(awoke_worker *wk)
{
	return worker_should_suspend(wk);
}

static err_type run_once(awoke_worker *wk)
{
	return wk->handler(wk);
}

static void *worker_run(void *context)
{
	err_type ret;
	awoke_worker_context *ctx = context;
	awoke_worker *wk = ctx->worker;

	if (!mask_exst(wk->features, WORKER_FEAT_PERIODICITY)) {
		//log_debug("worker %s not periodicity", wk->name);
		wk->handler();
	}

	do {
		
		if (mask_exst(wk->features, WORKER_FEAT_SUSPEND))
			worker_should_suspend(wk);
		
		ret = run_once(wk);
		
		worker_reference(wk);
		
		//log_debug("worker %s run once", wk->name);
		
		if (mask_exst(wk->features, WORKER_FEAT_TICK_USEC)) {
			usleep(wk->tick);
		} else if (mask_exst(wk->features, WORKER_FEAT_TICK_MSEC)){
			usleep(1000*wk->tick);
		} else {
			sleep(wk->tick);
		}
		
	} while (!worker_should_stop(wk));
}

awoke_worker *awoke_worker_create(char *name, uint32_t tick, 
		uint16_t features, err_type (*handler)(), void *data)
{
	int r;
	err_type ret;
	awoke_worker_context *context;

	awoke_worker *wk = mem_alloc_z(sizeof(awoke_worker));
	if (!wk) {
		return NULL;
	}

	context = mem_alloc_z(sizeof(awoke_worker_context));
	if (!context) {
		log_err("alloc context error");
		goto err;
	}
	context->worker = wk;

	wk->name = awoke_string_dup(name);
	if (!wk->name) {
		log_err("name dup error");
		goto err;
	}
	wk->tick = tick;
	wk->features = features;
	wk->_running = FALSE;
	wk->_force_stop = FALSE;
	wk->handler = handler;
	wk->data = data;

	if (mask_exst(wk->features, WORKER_FEAT_PIPE_CHANNEL)) {
		wk->evl = awoke_event_loop_create(8);
		if (!wk->evl) {
			goto err;
		}

		ret = awoke_event_pipech_create(wk->evl, &wk->pipe_channel);
		if (ret != et_ok) {
			log_err("worker %s pipe channel create error, ret %d", wk->name, ret);
			goto err;
		}
	}

	ret = worker_mutex_init(wk);
	if (ret != et_ok) {
		log_err("worker %s mutex init error, ret %d", wk->name, ret);
		goto err;
	}
	ret = worker_condition_init(wk);
	if (ret != et_ok) {
		log_err("worker %s mutex init error, ret %d", wk->name, ret);
		goto err;
	}

	worker_attr_init(&wk->attr);
	worker_attr_setdetachstate(&wk->attr, WORKER_CREATE_JOINABLE);

	if (mask_exst(wk->features, WORKER_FEAT_CUSTOM_DEFINE)) {
		r = worker_create(&wk->wid, &wk->attr, handler, (void *)context);
	} else {
		r = worker_create(&wk->wid, &wk->attr, worker_run, (void *)context);
	}
	if (r < 0) {
		log_err("worker %s create error, ret %d", wk->name, r);
		goto err;
	}

	log_debug("worker %s create ok, tick %d", wk->name, wk->tick);
	
	return wk;
	
err:
	worker_clean(&wk);
	return NULL;
}



/*
 * timer worker define -------------------------------- >>>>
 * 
 * -- Summary --
 * Timer worker is a set of smart timing task interfaces based 
 * on awoke_worker. Timer worker use struct 'awoke_tmwkr', it 
 * consists of a scheduler and several tasks. Scheduler is a
 * awoke_worker with special handler 'timer_scheduler' to sched
 * all the tasks on a list.
 * 
 * -- API -- 
 * @awoke_tmwkr_create : create a timer worker  
 * @awoke_tmwkr_start  : start a timer worker  
 * @awoke_tmwkr_stop   : stop a timer worker  
 * @awoke_tmwkr_clean  : clean a timer worker
 * @awoke_tmwkr_task_register : register a task to timer worker
 * 
 * -- Notes --
 * 1. timer worker will not run immediately at create time for 
 *    safety
 * 2. task's run time is not the clock register function set, 
 *    but the task->clock*twk->tick
 */
void awoke_tmtsk_clean(awoke_tmtsk **tsk)
{
	awoke_tmtsk *p;

	if (!tsk || !*tsk) 
		return;

	p = *tsk;	

	if (p->name)
		mem_free(p->name);

	mem_free(p);
	p = NULL;
}

void awoke_tmwkr_clean(awoke_tmwkr **twk)
{
	awoke_tmwkr *p;
	awoke_tmtsk *task, *temp;

	if (!twk || !*twk) 
		return;

	p = *twk;

	if (p->scheduler)
		awoke_worker_destroy(p->scheduler);

	list_for_each_entry_safe(task, temp, &p->task_queue, _head) {
		list_unlink(&task->_head);
		awoke_tmtsk_clean(&task);
	}

	mem_free(p);
	p = NULL;
}

err_type awoke_tmwkr_task_register(char *name, uint32_t clock, bool periodic,  
				bool (*can_sched)(struct _awoke_tmtsk *, struct _awoke_tmwkr *),
				err_type (*handle)(struct _awoke_tmtsk *, struct _awoke_tmwkr *), 
				awoke_tmwkr *twk)
{
	awoke_tmtsk *task;

	task = mem_alloc_z(sizeof(awoke_tmtsk));
	if (!task) 
		return et_nomem;

	task->name = awoke_string_dup(name);
	task->tid = twk->task_nr;
	task->tick = clock;
	task->clock = clock;
	task->can_sched = can_sched;
	task->handle = handle;
	task->periodic = periodic;

	twk->task_nr++;
	list_append(&task->_head, &twk->task_queue);

	return et_ok;
}

static err_type timer_scheduler(awoke_worker *wk)
{
	err_type ret = et_ok;
	awoke_tmtsk *task, *temp;
	awoke_tmwkr *twk = wk->data;
	
	if (list_empty(&twk->task_queue))
		return et_ok;

	list_for_each_entry_safe(task, temp, &twk->task_queue, _head) {
	
		task->tick--;
		bool _can_sched = FALSE;

		if (!task->tick) {
			if (task->can_sched != NULL) 
				_can_sched = task->can_sched(task, twk);
			else
				_can_sched = TRUE;
		}

		if (_can_sched) {
			ret = task->handle(task, twk);
			if ((ret != et_ok) && (task->err_proc != NULL)) {
				task->err_proc(task, twk);
			}
			if (!task->periodic) {
				list_unlink(&task->_head);
				awoke_tmtsk_clean(&task);
			}
			task->tick = task->clock;
		}
	}
}

void awoke_tmwkr_start(awoke_tmwkr *twk)
{
	return awoke_worker_start(twk->scheduler);
}

void awoke_tmwkr_stop(awoke_tmwkr *twk)
{
	return awoke_worker_stop(twk->scheduler);
}

awoke_tmwkr *awoke_tmwkr_create(char *name, uint32_t clock, uint16_t features, 
				err_type (*handler)(), void *data)
{
	err_type ret;
	awoke_tmwkr *twk;

	twk = mem_alloc_z(sizeof(awoke_tmwkr));
	if (!twk)
		return NULL;

	twk->data = data;
	twk->task_nr = 0;
	list_init(&twk->task_queue);

	if (!mask_exst(features, WORKER_FEAT_PERIODICITY))
		mask_push(features, WORKER_FEAT_PERIODICITY);

	if (!mask_exst(features, WORKER_FEAT_SUSPEND))
		mask_push(features, WORKER_FEAT_SUSPEND);

	/* scheduler create */
	if (mask_exst(features, WORKER_FEAT_CUSTOM_DEFINE)) {
		twk->scheduler = awoke_worker_create(name, clock, features, 
										     handler, (void *)twk);
	} else {
		twk->scheduler = awoke_worker_create(name, clock, features, 
											 timer_scheduler, (void *)twk);
	}

	if (!twk->scheduler) 
		goto err;

	return twk;
	
err:
	awoke_tmwkr_clean(&twk);
	return NULL;
}
