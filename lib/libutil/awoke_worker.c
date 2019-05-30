#include "awoke_worker.h"

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
	return wk->handler();
}

static void worker_run(void *context)
{
	err_type ret;
	awoke_worker_context *ctx = context;
	awoke_worker *wk = ctx->worker;

	if (!mask_exst(wk->features, WORKER_FEAT_PERIODICITY)) {
		log_debug("worker %s not periodicity", wk->name);
		wk->handler();
	}

	do {
		if (mask_exst(wk->features, WORKER_FEAT_SUSPEND))
			worker_should_suspend(wk);
		ret = run_once(wk);
		log_debug("worker %s run once", wk->name);
		if (mask_exst(wk->features, WORKER_FEAT_TICK_USEC)) {
			usleep(1000*wk->tick);
		} else {
			sleep(wk->tick);
		}
		
	} while (!worker_should_stop(wk));
}

awoke_worker *awoke_worker_create(char *name, uint32_t tick, 
		uint16_t features, err_type (*handler)())
{
	int r;
	err_type ret;
	awoke_worker_context *context;

	awoke_worker *wk = mem_alloc_z(sizeof(awoke_worker));
	if (!wk) {
		return NULL;
	}

	context = mem_alloc_z(sizeof(awoke_worker_context));
	context->worker = wk;

	wk->name = awoke_string_dup(name);
	wk->tick = tick;
	wk->features = features;
	wk->_running = FALSE;
	wk->_force_stop = FALSE;
	wk->handler = handler;

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

