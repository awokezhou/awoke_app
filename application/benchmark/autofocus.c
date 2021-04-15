
#include "autofocus.h"



static err_type autofocus_update(struct _bk_feedback_flow *flow);



static void autofocus_set_pos(struct _bk_autofocus_struct *af, uint32_t pos)
{
	af->pos = pos;
	mask_push(af->motor_cmd, AFOCUS_MOTOR_CMD_POS);
	bk_debug("set pos:%d", pos);
}

static void autofocus_set_step(struct _bk_autofocus_struct *af)
{
	mask_push(af->motor_cmd, AFOCUS_MOTOR_CMD_STEP);
}

static void autofocus_init(struct _bk_autofocus_struct *af)
{
	af->window = 3;
	af->fall = 0;
	af->gmax = 0;
	af->state = AFOCUS_S_NONE;
	af->pos = 0;
	af->step = 0;
	af->motor_cmd = 0x0;
	af->intr_events = 0x0;

	bk_fbkflow_init(&af->flow, FALSE);
	af->flow.data = af;
	af->flow.update = autofocus_update;
}

static void autofocus_handle(struct _bk_autofocus_struct *af)
{
	switch (af->state) {

	case AFOCUS_S_INIT_POS:
		if (mask_exst(af->motor_cmd, AFOCUS_MOTOR_CMD_POS_RSP) &&
			mask_exst(af->intr_events, AFOCUS_E_MOTOR_POS)) {
			mask_pull(af->motor_cmd, AFOCUS_MOTOR_CMD_POS_RSP);
			mask_pull(af->intr_events, AFOCUS_E_MOTOR_POS);
			bk_debug("motor pos ok");
			af->state = AFOCUS_S_STEP;
			bk_debug("afocus:step");
			bk_fbkflow_enable(&af->flow, TRUE);
		}
		break;

	case AFOCUS_S_STEP:
		if (mask_exst(af->motor_cmd, AFOCUS_MOTOR_CMD_STEP_RSP) &&
			mask_exst(af->intr_events, AFOCUS_E_MOTOR_STEP)) {
			mask_pull(af->intr_events, AFOCUS_E_MOTOR_STEP);
			af->step++;
			bk_debug("motor one step ok, step:%d", af->step);
			bk_fbkflow_notify(&af->flow, FBKFLOW_E_SET);
		}
		if (mask_exst(af->intr_events, AFOCUS_E_EXPO_START)) {
			mask_pull(af->intr_events, AFOCUS_E_EXPO_START);
			bk_fbkflow_notify(&af->flow, FBKFLOW_E_EXPO);
		}
		if (mask_exst(af->intr_events, AFOCUS_E_NEW_FRAME)) {
			mask_pull(af->intr_events, AFOCUS_E_NEW_FRAME);
			bk_fbkflow_notify(&af->flow, FBKFLOW_E_FRAME);
		}
		bk_fbkflow_run(&af->flow);
		break;

	case AFOCUS_S_FIN_POS:
		if (mask_exst(af->motor_cmd, AFOCUS_MOTOR_CMD_POS_RSP) &&
			mask_exst(af->intr_events, AFOCUS_E_MOTOR_POS)) {
			bk_debug("motor pos ok");
			mask_pull(af->motor_cmd, AFOCUS_MOTOR_CMD_POS_RSP);
			mask_pull(af->intr_events, AFOCUS_E_MOTOR_POS);
			af->state = AFOCUS_S_NONE;
			autofocus_init(af);
			bk_info("afocus finish");
		}		
		break;

	default:
		break;
	}
}

static void autofocus_start(struct _bk_autofocus_struct *af)
{
	if (af->state != AFOCUS_S_NONE)
		return;
	
	af->gmax = 0;
	af->fall = 0;
	bk_info("autofocus start");
	autofocus_set_pos(af, 576328);
	af->state = AFOCUS_S_INIT_POS;
}

static int data[10] = {13,15,17,19,21,20,19,18,16,11};

static err_type autofocus_update(struct _bk_feedback_flow *flow)
{
	int value;
	uint32_t finpos;
	static int count = 0;
	bk_autofocus_struct *af = flow->data;

	value = data[count++];
	bk_trace("value:%d", value);

	if (af->gmax <= value) {
		af->gmax = value;
		af->fall = 0;
		bk_trace("gmax update:%d", af->gmax);
		autofocus_set_step(af);
		flow->event = 0x0;
	} else {
		af->fall++;
		bk_trace("fall:%d", af->fall);
		
		if (af->fall >= af->window) {
			bk_info("find max:%d", af->gmax);
			bk_fbkflow_enable(&af->flow, FALSE);
			af->state = AFOCUS_S_FIN_POS;
			bk_debug("afocus:finpos");
			count = 0;
			finpos = 576328 - (af->step - af->fall)*1280;
			autofocus_set_pos(af, finpos);
		} else {
			autofocus_set_step(af);
		}
	}

	return et_ok;
}

static err_type fpga_work(void *context)
{
	awoke_worker_context *ctx = context;
	awoke_worker *wk = ctx->worker;
	bk_autofocus_struct *af = wk->data;

	sleep(2);
	bk_notice("fpga work start");

	while (!awoke_worker_should_stop(wk)) {

		mask_push(af->intr_events, AFOCUS_E_EXPO_START);
		bk_debug("expo start");
		sleep(3);		/* expotime */
		bk_debug("expo end");
		
		sleep(1);		/* frame generate */
		mask_push(af->intr_events, AFOCUS_E_NEW_FRAME);
		bk_debug("frame output");
		
		sleep(2);		/* idle */

	};

	return et_ok;
}

static err_type motor_work(void *context)
{
	awoke_worker_context *ctx = context;
	awoke_worker *wk = ctx->worker;
	bk_autofocus_struct *af = wk->data;

	sleep(2);
	bk_notice("motor work start");

	while (!awoke_worker_should_stop(wk)) {

		if (mask_exst(af->motor_cmd, AFOCUS_MOTOR_CMD_POS)) {
			mask_pull(af->motor_cmd, AFOCUS_MOTOR_CMD_POS);
			bk_trace("receive pos set");
			usleep(800*1000);
			bk_trace("ack signal pos set");
			mask_push(af->intr_events, AFOCUS_E_MOTOR_POS);
			usleep(200*1000);
			bk_trace("response pos set");
			mask_push(af->motor_cmd, AFOCUS_MOTOR_CMD_POS_RSP);
		}

		if (mask_exst(af->motor_cmd, AFOCUS_MOTOR_CMD_STEP)) {
			mask_pull(af->motor_cmd, AFOCUS_MOTOR_CMD_STEP);
			bk_trace("receive step set");
			usleep(400*1000);
			bk_trace("ack signal step set");
			mask_push(af->intr_events, AFOCUS_E_MOTOR_STEP);
			usleep(200*1000);
			bk_trace("response step set");
			mask_push(af->motor_cmd, AFOCUS_MOTOR_CMD_STEP_RSP);			
		}
	};

	return et_ok;
}


err_type benchmark_autofocus_test(int argc, char *argv[])
{
	int i;
	awoke_worker *wk1, *wk2;
	bk_autofocus_struct af;
	
	autofocus_init(&af);

	wk1 = awoke_worker_create("Motor", 1, WORKER_FEAT_CUSTOM_DEFINE, motor_work, &af);
	wk2 = awoke_worker_create("FPGA", 1, WORKER_FEAT_CUSTOM_DEFINE, fpga_work, &af);

	for (i=0; i<600; i++) {

		if (i == 20) {
			autofocus_start(&af);
		}

		if (i == 300) {
			autofocus_start(&af);
		}

		autofocus_handle(&af);

		usleep(200*1000);
	}

	awoke_worker_stop(wk1);
	awoke_worker_stop(wk2);
	
	awoke_worker_destroy(wk1);
	awoke_worker_destroy(wk2);

	return et_ok;
}
