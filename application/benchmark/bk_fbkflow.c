
#include "bk_fbkflow.h"
#include "benchmark.h"


static bool fbkflow_wait(struct _bk_feedback_flow *flow)
{
	bool vaild = FALSE;
	
	if (flow->wait) {
		return flow->wait(flow);
	}

	switch (flow->state) {

	case FBKFLOW_S_INIT:
		vaild = TRUE;
		bk_debug("state:idle");
		flow->state = FBKFLOW_S_IDLE;
		break;

	case FBKFLOW_S_IDLE:
		if (mask_exst(flow->event, FBKFLOW_E_SET)) {
			flow->state = FBKFLOW_S_SET;
			bk_debug("state:setup");
		}
		break;

	case FBKFLOW_S_SET:
		if (mask_exst(flow->event, FBKFLOW_E_EXPO)) {
			flow->state = FBKFLOW_S_EXPO;
			bk_debug("state:expo");
		} else {
			if (mask_exst(flow->event, FBKFLOW_E_FRAME)) {
				bk_err("invaild frame");
			}
		}
		break;

	case FBKFLOW_S_EXPO:
		if (mask_exst(flow->event, FBKFLOW_E_FRAME)) {
			flow->state = FBKFLOW_S_FRAME;
			bk_debug("state:frame");
		}
		break;

	case FBKFLOW_S_FRAME:
		bk_info("new frame come");
		vaild = TRUE;
		flow->state = FBKFLOW_S_IDLE;
		bk_debug("state:idle");
		break;

	default:
		break;
	}

	flow->event = 0x0;
	return vaild;
}

static err_type fbkflow_update(struct _bk_feedback_flow *flow)
{
	if (flow->update) {
		return flow->update(flow);
	}

	/* do nothing */

	return et_ok;
}

err_type bk_fbkflow_run(struct _bk_feedback_flow *flow)
{
	err_type ret = et_ok;

	if (!mask_exst(flow->flags, FBKFLOW_F_ENABLE)) {
		return et_ok;
	}
	
	if (fbkflow_wait(flow)) {
		return fbkflow_update(flow);
	} else {
		bk_trace("wait");
	}

	return ret;
}

void bk_fbkflow_enable(struct _bk_feedback_flow *flow, bool enable)
{
	if (!enable) {
		flow->event = 0x0;
		flow->state = FBKFLOW_S_INIT;
		mask_pull(flow->flags, FBKFLOW_F_ENABLE);
		bk_debug("flow disable");
	} else {
		mask_push(flow->flags, FBKFLOW_F_ENABLE);
		bk_debug("flow enable");
	}
}

void bk_fbkflow_notify(struct _bk_feedback_flow *flow, uint32_t event)
{
	if (mask_exst(flow->flags, FBKFLOW_F_ENABLE)) {
		mask_push(flow->event, event);
	}
}

void bk_fbkflow_init(struct _bk_feedback_flow *flow, bool enable)
{
	flow->flags = 0x0;
	flow->event = 0x0;
	flow->state = FBKFLOW_S_INIT;
	
	flow->wait = NULL;
	flow->update = NULL;

	if (enable) {
		mask_push(flow->flags, FBKFLOW_F_ENABLE);
	}
}

static err_type fbkflow_test_update(struct _bk_feedback_flow *flow)
{
	bk_info("algorithm start");
	sleep(1);
	bk_info("algorithm end");
	flow->event = 0x0;		/* ignore events within algorithm process */
	flow->state = FBKFLOW_S_SET;
	bk_debug("state:set");
	return et_ok;
}

static err_type fpga_work(void *context)
{
	awoke_worker_context *ctx = context;
	awoke_worker *wk = ctx->worker;
	bk_fbkflow *flow = wk->data;

	sleep(2);
	bk_notice("fpga start work");

	while (!awoke_worker_should_stop(wk)) {

		bk_fbkflow_notify(flow, FBKFLOW_E_EXPO);
		bk_trace("expo start");
		sleep(3);		/* expotime */
		bk_trace("expo end");
		
		sleep(1);		/* frame generate */
		bk_fbkflow_notify(flow, FBKFLOW_E_FRAME);
		bk_trace("frame output");
		
		sleep(2);		/* idle */

	};

	return et_ok;
}

err_type benchmark_fbkflow_test(int argc, char *argv[])
{
	int i;
	bk_fbkflow autofocus_flow;
	awoke_worker *wk;

	bk_fbkflow_init(&autofocus_flow, TRUE);
	autofocus_flow.update = fbkflow_test_update;

	wk = awoke_worker_create("FPGA", 1, WORKER_FEAT_CUSTOM_DEFINE, fpga_work, &autofocus_flow);

	for (i=0; i<200; i++) {

		/*
		if (i == 40) {
			bk_fbkflow_enable(&autofocus_flow, TRUE);
		} else if (i==120) {
			bk_fbkflow_enable(&autofocus_flow, FALSE);
		} else if (i==180) {
			bk_fbkflow_enable(&autofocus_flow, TRUE);
		}
		*/
		
		bk_fbkflow_run(&autofocus_flow);
		usleep(200*1000);
	}

	awoke_worker_stop(wk);
	awoke_worker_destroy(wk);

	return et_ok;
} 