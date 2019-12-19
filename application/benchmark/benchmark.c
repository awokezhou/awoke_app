
#include <stdio.h>
#include <time.h> 
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <fcntl.h>
#include <pthread.h> 
#include <sys/types.h>
#include <sys/shm.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <linux/input.h>
#include <arpa/inet.h>
#include <math.h>
#include <sys/stat.h>
#include <unistd.h>   
#include <assert.h>
#include <getopt.h>

#include "benchmark.h"

static void usage(int ex)
{	
	printf("usage : benchamrk [option]\n\n");

    printf("    -waitev-test,    --file\t\tread data from file\n");
    printf("    -condaction-test,    --input\t\tread data from input\n");

	EXIT(ex);
}

static err_type wait_test1_fn(awoke_wait_ev *ev)
{
	waitev_test_t *p = ev->data;

	p->count++;

	log_debug("count %d", p->count);

	if (p->count >= 5) {
		log_debug("count > 5, finish");
		return et_waitev_finish;
	}

	return et_ok;
}

err_type awoke_wait_test(void *data)
{
	log_debug("awoke_wait_test");
	waitev_test_t t;
	t.count = 0;
	awoke_wait_ev *ev = awoke_waitev_create("wait-test", 5, WAIT_EV_F_SLEEP, wait_test1_fn, NULL, (void *)&t);
	if (!ev) {
		log_err("ev create error");
		return et_waitev_create;
	}

	return awoke_waitev(ev);
}

static void pipech_msg_header_dump(pipech_msg_header *hdr)
{
	log_info("pipe channel header ---->");
	log_info("src:%d", hdr->src);
	log_info("dst:%d", hdr->dst);
	log_info("type:0x%x", hdr->msg_type);
	log_info("f_event:%d", hdr->f_event);
	log_info("f_request:%d", hdr->f_request);
	log_info("f_response:%d", hdr->f_response);
	log_info("f_broadcast:%d", hdr->f_broadcast);
	log_info("word:0x%x", hdr->word);
	log_info("data_len:%d", hdr->data_len);
}

err_type benchmark_event_chn_work(awoke_worker *wk)
{
	pipech_msg_header w_header;
	pipech_msg_header r_header;
	awoke_event *event;
	event_channel_test_t *t = wk->data;

	log_debug("worker %s run", wk->name);

	memset(&w_header, 0x0, sizeof(w_header));
	memset(&r_header, 0x0, sizeof(w_header));
	w_header.src = wk->wid;
	w_header.dst = 0;
	w_header.msg_type = 0x05;
	w_header.f_event = 1;
	w_header.data_len = 0;
	
	awoke_event_wait(wk->evl, 1000);
	awoke_event_foreach(event, wk->evl) {
		if (event->type == EVENT_NOTIFICATION) {
			read(event->fd, &r_header, sizeof(r_header));
			if (event->fd == wk->pipe_channel.ch_r) {
				pipech_msg_header_dump(&r_header);
			}
		}
	}

	write(t->pipe_channel.ch_w, &w_header, sizeof(w_header));
	log_debug("worker %s send msg to main", wk->name);
}

err_type benchmark_event_channel_test(void *data)
{
	int rc;
	pipech_msg_header w_header;
	pipech_msg_header r_header;
	err_type ret;
	awoke_event *event;
	awoke_event_loop *evl;
	event_channel_test_t *t;

	t = mem_alloc_z(sizeof(event_channel_test_t));

	log_debug("event channel test");

	t->evl = awoke_event_loop_create(8);
	if (!t->evl) {
		log_err("event loop create error");
		return et_evl_create;
	}

	ret = awoke_event_pipech_create(t->evl, &t->pipe_channel);
	if (ret != et_ok) {
		log_err("event channel create error, ret %d", ret);
		return et_evl_create;
	}
	log_debug("main event channel create ok");

	awoke_worker *wk = awoke_worker_create("evchn", 3, 
										   WORKER_FEAT_PERIODICITY|WORKER_FEAT_PIPE_CHANNEL,
										   benchmark_event_chn_work,
										   (void *)t);
	log_debug("worker create ok");

	memset(&w_header, 0x0, sizeof(w_header));
	memset(&r_header, 0x0, sizeof(w_header));
	w_header.src = 0;
	w_header.dst = wk->wid;
	w_header.msg_type = 0x06;
	w_header.f_event = 1;
	w_header.data_len = 0;

	while (1) {
		awoke_event_wait(t->evl, 2000);
		awoke_event_foreach(event, t->evl) {
			if (event->type == EVENT_NOTIFICATION) {
				//log_debug("NOTIFICATION");
				rc = read(event->fd, &r_header, sizeof(r_header));
				if (rc < 0) {
					log_err("read error");
					continue;
				}
				if (event->fd == t->pipe_channel.ch_r) {
					pipech_msg_header_dump(&r_header);
					write(wk->pipe_channel.ch_w, &w_header, sizeof(w_header));
					log_debug("main send msg to worker %s", wk->name);
				}
			}
		}
	}
}

err_type *benchmark_test_work(void *data)
{
	log_debug("run");
}

err_type benchmark_test_worker_wait(struct _awoke_wait_ev *ev)
{
	if (ev->_reference >= 10) {
		return et_waitev_finish;
	}

	return et_ok;
}

err_type benchmark_worker_test(void *data)
{
	awoke_worker *wk = awoke_worker_create("worker-test", 2, WORKER_FEAT_PERIODICITY|WORKER_FEAT_SUSPEND,
								  		   benchmark_test_work, NULL);
	awoke_worker_start(wk);

	sleep(10);

	awoke_worker_stop(wk);
	awoke_worker_destroy(wk);
}

static void *pthread_test_handle(void *arg)
{
	printf("pthread start!\n");

	sleep(2);

	return NULL;
} 

err_type benchmark_pthread_test(void *data)
{
	pthread_t tidp;

	if ((pthread_create(&tidp, NULL, pthread_test_handle, NULL)) == -1) {
		printf("create error!\n");
		return -1;
	}

	sleep(1);

	printf("mian continue!\n");

	if (pthread_join(tidp, NULL)) {
		printf("thread is not exit...\n");
		return -2;
	}

	return et_ok;
}

err_type benchmark_bitflag_test(void *data)
{
	typedef struct _bitflag {
		uint16_t f_active,
				 f_online,
				 f_protect,
				 f_multi;
	} bitflag;

	bitflag flag;

	flag.f_active = TRUE;
	flag.f_online = TRUE;

	if (!flag.f_active) {
		log_debug("not active");
	} else {
		log_debug("active");
	}

	if (!flag.f_online) {
		log_debug("not online");
	} else {
		log_debug("online");
	}

	if (!flag.f_protect) {
		log_debug("not protect");
	} else {
		log_debug("protect");
	}

	if (!flag.f_multi) {
		log_debug("not multi");
	} else {
		log_debug("multi");
	}

}

err_type worker_lock_test(awoke_worker *wk)
{
	err_type ret;
	lock_test_t *x = wk->data;

	log_debug("%s acquire lock", wk->name);

	ret = awoke_lock_acquire_timeout(x->sem_lock, 200);
	if (ret != et_ok) {
		log_err("acquire lock error");
	} else {
		log_debug("acquire lock ok");
		awoke_lock_release(x->sem_lock);
	}
}

err_type benchmark_lock_test(void *data)
{
	lock_test_t x;
	x.mux_lock = awoke_lock_create(AWOKE_LOCK_T_MUX);
	if (!x.mux_lock) {
		log_err("mux lock create error");
		return et_ok;
	}

	x.sem_lock = awoke_lock_create(AWOKE_LOCK_T_SEM);
	if (!x.sem_lock) {
		log_err("sem lock create error");
		return et_ok;
	}

	log_debug("main acquire lock");
	awoke_lock_acquire(x.sem_lock, AWOKE_LOCK_F_BLOCK);

	x.wk = awoke_worker_create("lock-test", 2, 
							   WORKER_FEAT_PERIODICITY,
							   worker_lock_test,
							   (void *)&x);
	if (!x.wk) {
		log_err("worker create error");
		return et_ok;
	}

	sleep(10);
	awoke_lock_release(x.sem_lock);
	log_debug("main release lock");
	sleep(10);

	awoke_worker_stop(x.wk);
	awoke_worker_destroy(x.wk);
	awoke_lock_clean(&x.mux_lock);
	awoke_lock_clean(&x.sem_lock);
	return et_ok;
}

static err_type timer_worker_read_obd(awoke_tmtsk *tsk, awoke_tmwkr *twk)
{
	log_debug("%s run", tsk->name);

	return et_ok;
}

static err_type timer_worker_read_eng(awoke_tmtsk *tsk, awoke_tmwkr *twk)
{
	log_debug("%s run", tsk->name);
	return et_ok;
}

static err_type timer_worker_getvin(awoke_tmtsk *tsk, awoke_tmwkr *twk)
{
	log_debug("%s run", tsk->name);
	return et_ok;
}

static err_type timer_worker_sched_end(awoke_tmwkr *twk)
{
	timer_worker_test_t *t = twk->data;
	t->x++;
	return et_ok;
}

static err_type benchmark_timer_worker_test(void *data)
{
	err_type ret;
	timer_worker_test_t t;

	t.x = 0;
	t.twk = awoke_tmwkr_create("timer-test", 10, 
							   WORKER_FEAT_TICK_MSEC,
							   NULL, 
							   timer_worker_sched_end, 
							   (void *)&t);
	if (!t.twk) {
		log_err("timer worker create error");
		return et_ok;
	}

	log_debug("timer worker create ok");

	ret = awoke_tmwkr_task_register("read_obd", 300, TRUE, NULL, 
						 	  	    timer_worker_read_obd, t.twk);
	ret = awoke_tmwkr_task_register("read_eng", 500, TRUE, NULL, 
						 	  	    timer_worker_read_eng, t.twk);
	ret = awoke_tmwkr_task_register("getvin", 1000, FALSE, NULL, 
						 	  	    timer_worker_read_eng, t.twk);

	awoke_tmwkr_start(t.twk);
	
	sleep(10);
	log_debug("x %d", t.x);

	awoke_tmwkr_stop(t.twk);

	awoke_tmwkr_clean(&t.twk);
	return et_ok;
}

static uint32_t _queue_zero_filter_uint32(awoke_queue *q)
{

	int idx = 0;
	int size = 0;
	int zero_nr = 0;
	int zero_percent;
	uint32_t *data;
	uint32_t average = 0;

	size = awoke_queue_size(q);
	
	awoke_queue_foreach(q, data, uint32_t) {
		idx++;
		log_debug("data %d", *data);
		if ((*data == 0) && ((idx!=1) && (idx!=size))) {
			log_debug("zero_nr+=");
			zero_nr++;
		}
		average += *data;
	}

	log_debug("zero_nr %d", zero_nr);

	average = average/(size-zero_nr);

	log_debug("average %d", average);

	zero_percent = zero_nr*100/size;

	log_debug("zero_percent %d", zero_percent);
	
	if (zero_percent > 10) {
		average = 0;
		awoke_queue_foreach(q, data, uint32_t) {
			average += *data;
		}
		average = average/size;
	} 

	return average;
}

static err_type benchmark_queue_zero_filter_test(void *data)
{
	int i;
	uint32_t *u;
	uint32_t average;
	uint32_t x[8] = {0, 0, 2, 8, 0, 3, 7, 0};
	char buff[256];
	build_ptr bp = build_ptr_init(buff, 256);
	
	awoke_queue *q = awoke_queue_create(sizeof(uint32_t), 10, 0x0);
	if (!q) {
		log_err("queue create error");
		return et_nomem;
	}

	for (i=0; i<8; i++) {
		awoke_queue_enq(q, &x[i]);
	}

	build_ptr_string(bp, "queue(10): [");
	awoke_queue_foreach(q, u, uint32_t) {
		build_ptr_format(bp, "%d ", *u);
	}
	build_ptr_string(bp, "]");
	log_debug("%.*s", bp.len, bp.head);

	average = _queue_zero_filter_uint32(q);

	log_debug("average %d", average);

	awoke_queue_clean(&q);

	return et_ok;
}

static err_type benchmark_vin_parse_test(void *data)
{
	char *vin = data;
	vininfo vinfo;
	vin_parse(vin, &vinfo);
	vininfo_dump(&vinfo);
	return et_ok;
}

int main(int argc, char *argv[])
{
	int opt;
	void *data;
	int loglevel = LOG_DBG;
	int logmode = LOG_TEST;
	bencmark_func bmfn = NULL;

	log_id("benchmark");

	static const struct option long_opts[] = {
		{"loglevel",			required_argument,	NULL,	'l'},
		{"logmode",				required_argument,	NULL,	'm'},
        {"waitev-test",			no_argument,		NULL,   arg_waitev_test},
        {"condaction-test",  	no_argument,  		NULL,   arg_condaction_test},
        {"event-channel",		no_argument,		NULL,	arg_event_channel},
        {"worker",				no_argument,		NULL,	arg_worker},
        {"pthread",				no_argument,		NULL,	arg_pthread},
		{"bitflag-test",		no_argument,		NULL,	arg_bitflag},
		{"lock-test",			no_argument,		NULL,	arg_lock},
		{"timer-worker-test",	no_argument,		NULL,	arg_timer_worker},
		{"queue-zero-filter",	no_argument, 		NULL, 	arg_queue_zero_filter},
		{"vin-parse-test",		required_argument,	NULL,	arg_vin_parse_test},
        {NULL, 0, NULL, 0}
    };	

	while ((opt = getopt_long(argc, argv, "l:m:?h-", long_opts, NULL)) != -1)
    {
        switch (opt)
        {
        	case 'l':
				loglevel = atoi(optarg);
				break;

        	case 'm':
				logmode = atoi(optarg);
				break;
			
            case arg_waitev_test:
                bmfn = awoke_wait_test;
				break;
                
            case arg_condaction_test:
                break;

			case arg_event_channel:
				bmfn = benchmark_event_channel_test;
				break;

			case arg_worker:
				bmfn = benchmark_worker_test;
				break;

			case arg_pthread:
				bmfn = benchmark_pthread_test;
				break;

			case arg_bitflag:
				bmfn = benchmark_bitflag_test;
				break;

			case arg_lock:
				bmfn = benchmark_lock_test;
				break;

			case arg_timer_worker:
				bmfn = benchmark_timer_worker_test;
				break;

			case arg_queue_zero_filter: 
				bmfn = benchmark_queue_zero_filter_test;
				break;

			case arg_vin_parse_test:
				data = optarg;
				bmfn = benchmark_vin_parse_test;
				break;
				
            case '?':
            case 'h':
			case '-':
            default:
                usage(AWOKE_EXIT_SUCCESS);
        }
    }

	log_level(loglevel);
	log_mode(logmode);

	bmfn(data);

	return 0;
}
