#include <getopt.h>

#include "drv_bhv_core.h"
#include "drv_bhv_calculate.h"
#include "drv_bhv_detect.h"


//static struct sembuf sem_up[1] = {{0, -1, IPC_NOWAIT | SEM_UNDO}}; // set sem_up
static struct sembuf sem_up[1] = {{0, -1, SEM_UNDO}}; // set sem_up
static struct sembuf sem_dw[1] = {{0, +1, SEM_UNDO}}; // set sem_dn


void bhv_help(int ex)
{
	printf("usage : drv_bhv [option]\n\n");

    printf("    -d,      --daemon\t\t\trun web_httpd as daemon\n");
    printf("    -l debug level, 5:debug 4:info 3:warning 2:error 1:bug\n");

	EXIT(ex);
}

void bhv_param_clean(bhv_param **param)
{
    bhv_param *p;

    if (!*param || !param)
        return;

    p = *param;

    mem_free(p);
    p = NULL;
    return;
}

void bhv_manager_clean(bhv_manager **manager)
{
    bhv_manager *p;
	bhv_param *param; 

    if (!*manager || !manager)
        return;

    p = *manager;
	param = p->param;

	bhv_detect_clean(manager);
	bhv_calculate_clean(manager);
	bhv_param_clean(&param);

   	mem_free(p);
    p = NULL;
    return;    
}

bhv_param *bhv_param_create()
{	
	bhv_param *param = NULL;

	param = mem_alloc_z(sizeof(bhv_param));
	if (!param)
		return NULL;

	param->daemon = FALSE;
	param->dlevel = LOG_ERR;
	return param;
}

bhv_param * bhv_param_parse(int argc, char *argvs[])
{
	int opt;
    bhv_param *param = NULL;

	param = bhv_param_create();
    if (!param)
    {
       	log_err("param create error");
        return NULL;
    }

	static const struct option long_opts[] = {
        {"daemon",      no_argument,        NULL,   'd'},
        {"dlevel",   	required_argument,  NULL,   'l'},
        {""},
        {NULL, 0, NULL, 0}
    };

	while ((opt = getopt_long(argc, argvs, "dl:?h", long_opts, NULL)) != -1)
    {
        switch (opt)
        {
            case 'd':
                param->daemon = TRUE;
                break;
                
            case 'l':
                param->dlevel = atoi(optarg);
                break;

            case '?':
            case 'h':
            default:
                bhv_help(AWOKE_EXIT_SUCCESS);
        }
    }

	return param;
}

bhv_manager *bhv_manager_create(bhv_param *param)
{
	bhv_manager *manager;

	if (!param)
        return NULL;

	manager = mem_alloc_z(sizeof(bhv_manager));
    if (!manager)
        return NULL;

	manager->param = param;

	list_init(&manager->worker_list);
	list_init(&manager->calc_list);
	list_init(&manager->dtct_list);

	return manager;
}

void bhv_run_daemon(bhv_manager *manager)
{
    pid_t pid;

	if (!manager->param->daemon)
		return;

    pid = fork();

    switch (pid)
    {
        case -1:                        
            log_err("fork error!");
            return;

        case 0:                         // child
            break;

        default:                        // father
            EXIT(0);                
    }

    pid = setsid();
    if (pid < 0)
    {
        log_err("setsid error");
        return;
    }

    log_debug("daemon ok");
    return;
}

err_type acc_listener(struct _bhv_worker *worker, struct _bhv_manager *manager)
{
	vector vec;
	bhv_calc *calc;
	
	memset(&vec, 0x0, sizeof(vector));
	vec.x = (double)awoke_random_int(10, 1);
	vec.x++;
	vec.y = (double)awoke_random_int(11, 1);
	vec.z = (double)awoke_random_int(12, 1);

	log_info("%s vector(%lf %lf %lf)", worker->name, vector_print(vec));

	list_for_each_entry(calc, &manager->calc_list, _head) {
		if (worker->calc_flag & calc->mask) {
			if (calc->calc_handler != NULL)
				calc->calc_handler((void *)&vec, calc, manager);
		}
	}
	
	return et_ok;
}

err_type gyr_listener(struct _bhv_worker *worker, struct _bhv_manager *manager)
{
	vector vec;
	bhv_calc *calc;
	
	memset(&vec, 0x0, sizeof(vector));
	vec.x = awoke_random_int(10, 1);
	vec.x++;
	vec.y = awoke_random_int(11, 1);
	vec.z = awoke_random_int(12, 1);

	log_info("%s vector(%lf %lf %lf)", worker->name, vector_print(vec));

	list_for_each_entry(calc, &manager->calc_list, _head) {
		if (worker->calc_flag & calc->mask) {
			if (calc->calc_handler != NULL) {
				calc->calc_handler((void *)&vec, calc, manager);
			}
		}
	}

	return et_ok;
}

err_type gps_listener(struct _bhv_worker *worker, struct _bhv_manager *manager)
{
	log_info("gps_listener");
	return et_ok;
}

static inline void scheduler_lock_acquire(bhv_scheduler *p)
{
    if (semop(p->sem_id, sem_up, 1) == -1) {
        log_err("lock acquire error");
   	}
}

static inline void scheduler_lock_release(bhv_scheduler *p)
{
    if (semop(p->sem_id, sem_dw, 1) == -1) {
        log_err("lock release error");
    }
}

void timer_scheduler(void *context)
{
	err_type ret = et_ok;
	bhv_scheduler_context *ctx = context;
	struct _bhv_manager *manager = ctx->manager;
	struct _bhv_scheduler *scheduler = &manager->scheduler;
	struct _bhv_worker *worker;

	while(scheduler->loop) {
		list_for_each_entry(worker, &manager->worker_list, _head) {
			worker->tick--;
			if (!worker->tick) {

				scheduler_lock_acquire(scheduler);
				ret = worker->worker_handler(worker, manager);
				if (et_ok != ret) {
					log_err("%s handle error, ret %d", worker->name, ret);
				}
				scheduler_lock_release(scheduler);

				worker->tick = worker->tick_int;
			}
		}
		scheduler_sleep(scheduler->tick);
	}

	return;
}

err_type bhv_scheduler_start(bhv_manager *manager)
{
	bhv_scheduler_id sid;
	bhv_scheduler_attr attr;
	bhv_scheduler_context *context;
	bhv_scheduler *scheduler = &manager->scheduler;
	scheduler->loop = TRUE;

	context = mem_alloc_z(sizeof(bhv_scheduler_context));
	context->manager = manager;

	bhv_scheduler_attr_init(&attr);
	bhv_scheduler_attr_setdetachstate(&attr, SCHEDULER_CREATE_JOINABLE);
	if (bhv_scheduler_create(&sid, &attr, scheduler->scheduler, 
		                     (void *)context) != 0) {
		log_err("scheduler create error");
		return et_fail;
	}

	return et_ok;
}

err_type bhv_scheduler_worker_register(const char *name, 
				err_type (*handler)(struct _bhv_worker *, struct _bhv_manager *),
				int calc_mask,
				int tick_int,
				int tick_val,
				bhv_manager *manager)
{
	bhv_worker *worker;
	bhv_worker *new_worker;

	if (!name || !handler || !manager)
		return et_param;

	list_for_each_entry(worker, &manager->worker_list, _head) {
		if (!strcmp(name, worker->name)) {
			log_debug("worker exist");
			return et_exist;
		}
	}

	new_worker = mem_alloc_z(sizeof(bhv_worker));
	if (!new_worker)
		return et_nomem;
	
	new_worker->tick_int = tick_int;
	new_worker->tick_val = tick_val;
	new_worker->tick = tick_val;
	new_worker->calc_flag = calc_mask;
	new_worker->name = awoke_string_dup(name);
	new_worker->worker_handler = handler;
	list_append(&new_worker->_head, &manager->worker_list);
	return et_ok;
}

err_type scheduler_lock_init(bhv_scheduler *scheduler)
{
	int sem_id;
	err_type ret = et_ok;
	union semun sem_union;
	
	sem_id = semget(LOCK_KEY_ID, 1, 0666 | IPC_CREAT);
	if (sem_id < 0) {
		log_err("create lock error");
		ret = et_sem_create;
		goto err;
	}

	sem_union.val = 1;
	if(semctl(sem_id, 0, SETVAL, sem_union) == -1) {
		log_err("init lock error");
		ret = et_sem_init;
		goto err;
	}

	scheduler->sem_id = sem_id;

	return ret;

err:
	return ret;
}

err_type bhv_scheduler_init(bhv_manager *manager)
{
	err_type ret = et_ok;
	manager->scheduler.tick = TICK_NORMAL;
	manager->scheduler.scheduler = timer_scheduler;

	ret = scheduler_lock_init(&manager->scheduler);
	if (et_ok != ret)
		return ret;
	
	bhv_scheduler_worker_register("[worker-acc]", acc_listener, 
								  CALC_HEAD_ACC|CALC_ACC_ANGLE,
								  2, 2, manager);
	

	bhv_scheduler_worker_register("[worker-gyr]", gyr_listener, 
							      CALC_GYR_CORNER|CALC_GYR_ANGLE, 
							      2, 2, manager);
	
	/*
	bhv_scheduler_worker_register("gps-listener", gps_listener, 
							      CALC_GPS_ACC|CALC_GPS_CORNER, 
							      10, 10, manager);
	*/

	return bhv_scheduler_start(manager);
}

static void sched_worker_free(bhv_worker **worker)
{
	bhv_worker *p;

	if (!worker || !*worker)
		return;

	p = *worker;
	if (p->name)
		mem_free(p->name);
	mem_free(p);
	p = NULL;
	return;
}

void sched_lock_free(bhv_scheduler *sched)
{
	union semun sem_union;
	semctl(sched->sem_id, 0, IPC_RMID, sem_union);
}

void bhv_scheduler_clean(bhv_manager *manager)
{
	bhv_worker *temp;
	bhv_worker *worker;
	bhv_scheduler *sched = &manager->scheduler;

	sched_lock_free(sched);

	list_for_each_entry_safe(worker, temp, &manager->worker_list, _head) {
		list_unlink(&worker->_head);
		sched_worker_free(&worker);
	}
}

err_type bhv_init(bhv_manager *manager)
{
	err_type ret = et_ok;
	bhv_param *param = manager->param;

	log_set(param->dlevel);
	
	//bhv_signal_init(manager);

	bhv_run_daemon(manager);

	ret = bhv_calculate_init(manager);
	if (et_ok != ret) {
		log_err("calculate init error, ret %d", ret);
		return ret;
	}

	ret = bhv_detect_init(manager);
	if (et_ok != ret) {
		log_err("detect init error, ret %d", ret);
		return ret;
	}

	ret = bhv_scheduler_init(manager);
	if (et_ok != ret) {
		log_err("scheduler init error, ret %d", ret);
		return ret;
	}

	return ret;
}

err_type bhv_main_loop(bhv_manager *manager)
{

	bhv_scheduler *sched = &manager->scheduler;

	while(1) {

		sleep(1);

		scheduler_lock_acquire(sched);
		log_err("bhv_detect get lock");
		bhv_detect(manager);
		scheduler_lock_release(sched);
		
	}
	return et_ok;
}

int main(int argc, char *argv[])
{
	err_type ret = et_ok;
	bhv_param *param;
	bhv_manager *manager;

	param = bhv_param_parse(argc, argv);
	if (!param) {
		log_err("param parse error, will exit...");
		EXIT(AWOKE_EXIT_FAILURE);
	}

	manager = bhv_manager_create(param);
	if (!manager) {
		log_err("manager create error, will exit...");
		bhv_param_clean(&param);
		EXIT(AWOKE_EXIT_FAILURE);
	}

	ret = bhv_init(manager);
	if (et_ok != ret) {
		log_err("manager create error, will exit...");
		goto err;
	}

	ret = bhv_main_loop(manager);
	log_info("main loop return, ret %d");

err:
    bhv_manager_clean(&manager);
    EXIT(AWOKE_EXIT_SUCCESS);
}
