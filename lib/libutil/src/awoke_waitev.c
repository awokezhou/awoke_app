
#include <time.h>
#include "awoke_waitev.h"
#include "awoke_log.h"

static bool ev_should_stop(awoke_wait_ev *ev)
{
	if (mask_exst(ev->flag, WAIT_EV_F_CUSTOM_STOP) && ev->stop_fn) {
		return ev->stop_fn(ev);
	} else if ((!ev->stop_fn) && (ev->ret == et_waitev_finish)) {
		return TRUE;
	} 

	if (mask_exst(ev->flag, WAIT_EV_F_SLEEP)) {
		sleep(ev->tick);
	}

	return FALSE;
}

err_type awoke_waitev(awoke_wait_ev *ev)
{
	err_type ret;
	
	do {
		
		ret = ev->run_once(ev);
		log_debug("waitev %s run once, ret %d", ev->name, ret);
		ev->ret = ret;

		if (mask_exst(ev->flag, WAIT_EV_F_REFERENCE))
			ev->_reference++;
		
	} while(!ev_should_stop(ev));

	return ret;
}

awoke_wait_ev *awoke_waitev_create(char *name, int tick, uint8_t flags, 
			err_type (*run_once)(struct _awoke_wait_ev *),
			bool (*stop_fn)(struct _awoke_wait_ev *),
			void *data)
{
	awoke_wait_ev *ev;

	if (!name || !run_once)
		return NULL;

	if (mask_exst(flags, WAIT_EV_F_CUSTOM_STOP) && (!stop_fn))
		return NULL;

	ev = mem_alloc_z(sizeof(awoke_wait_ev));
	if (!ev)
		return NULL;

	ev->ret = et_ok;
	ev->name = awoke_string_dup(name);
	ev->tick = tick;
	ev->_reference = 0;
	ev->flag = flags;
	ev->run_once = run_once;
	ev->stop_fn = NULL;

	if (data != NULL)
		ev->data = data;

	if (stop_fn != NULL)
		ev->stop_fn = stop_fn;

	return ev;
}
