
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

err_type awoke_wait_test()
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

err_type benchmark_event_chn_work(awoke_worker *wk)
{
	uint8_t r_msg = 0x00;
	uint8_t w_msg = 0x45;
	awoke_event *event;
	event_channel_test_t *t = wk->data;

	log_debug("worker %s run", wk->name);

	awoke_event_wait(wk->evl, 1000);
	awoke_event_foreach(event, wk->evl) {
		if (event->type == EVENT_NOTIFICATION) {
			read(event->fd, &r_msg, sizeof(r_msg));
			if (event->fd == wk->pipe_channel.ch_r) {
				log_info("notification: 0x%x", r_msg);
			}
		}
	}

	write(t->pipe_channel.ch_w, &w_msg, sizeof(w_msg));
	log_debug("worker %s send msg to main", wk->name);
}

err_type benchmark_event_channel_test()
{
	int rc;
	uint8_t r_msg = 0x00;
	uint8_t w_msg= 0x45;
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

	while (1) {
		awoke_event_wait(t->evl, 2000);
		awoke_event_foreach(event, t->evl) {
			if (event->type == EVENT_NOTIFICATION) {
				//log_debug("NOTIFICATION");
				rc = read(event->fd, &r_msg, sizeof(r_msg));
				if (rc < 0) {
					log_err("read error");
					continue;
				}
				if (event->fd == t->pipe_channel.ch_r) {
					log_info("notification: 0x%x", r_msg);
					write(wk->pipe_channel.ch_w, &w_msg, sizeof(w_msg));
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

err_type benchmark_worker_test()
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

err_type benchmark_pthread_test()
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

int main(int argc, char *argv[])
{
	int opt;
	bencmark_func bmfn = NULL;

	log_id("benchmark");
	log_mode(LOG_SYS);
	log_debug("bencmark");

	static const struct option long_opts[] = {
        {"waitev-test",			no_argument,	NULL,   arg_waitev_test},
        {"condaction-test",  	no_argument,  	NULL,   arg_condaction_test},
        {"event-channel",		no_argument,	NULL,	arg_event_channel},
        {"worker",				no_argument,	NULL,	arg_worker},
        {"pthread",				no_argument,	NULL,	arg_pthread},
        {NULL, 0, NULL, 0}
    };	

	while ((opt = getopt_long(argc, argv, "?h-", long_opts, NULL)) != -1)
    {
        switch (opt)
        {
            case arg_waitev_test:
                return awoke_wait_test();
                
            case arg_condaction_test:
                break;

			case arg_event_channel:
				return benchmark_event_channel_test();

			case arg_worker:
				return benchmark_worker_test();

			case arg_pthread:
				return benchmark_pthread_test();

            case '?':
            case 'h':
			case '-':
            default:
                usage(AWOKE_EXIT_SUCCESS);
        }
    }

	return 0;
}
