#define namespace_awoke
#define namespace_queue

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
#include "awoke_log.h"

#include "bk_filecache.h"


static void usage(int ex)
{
	
	printf("usage : benchamrk [option]\n\n");

    printf("\t--waitev-test\t\t\twaitev test\n");
    printf("\t--condaction-test\t\tcondition&action test\n");
	printf("\t--event-channel\t\t\tevent channel test\n");
	printf("\t--worker\t\t\tworker test\n");
	printf("\t--pthread\t\t\tpthread test\n");
	printf("\t--bitflag-test\t\t\tbitflag test\n");
	printf("\t--lock-test\t\t\tlock test\n");
	printf("\t--timer-worker-test\t\ttimer worker test\n");
	printf("\t--queue-zero-filter\t\tqueue zero filter\n");
	printf("\t--vin-parse-test <vin>\t\tvinparser test, <vin>:input vin\n");
	printf("\t--http-request-test\t\thttp request test\n");
	printf("\t--sscanf-test\t\tsscanf test\n");
	printf("\t--buffchunk-test\t\tbuffchunk test\n");
	printf("\t--valist-test\t\tvalist test\n");
	printf("\t--timezero-test\t\ttimezero test\n");
	printf("\t--fastlz-test\t\tfastlz test\n");
	EXIT(ex);
}

static void http_requset_usage(int ex) 
{
	printf("usage : benchmark --http-request-test\n\n");

	printf("\t-m/--methd\t\thttp request method, GET/POST\n");
	printf("\t-p/--protocol\t\thttp requset protocol, HTTP/1.1...\n");
	printf("\t-u/--uri\t\thttp requset uri\n");
	printf("\t-b/--body\t\thttp requset body\n");
	printf("\t--Content-Type\t\theader Content-Type\n");
	printf("\t--Authorization\t\theader Authorization\n");

	printf("default:benchmark --http-request-test -u www.baidu.com -m POST -p HTTP/1.1 -b helloworld\n");
	EXIT(ex);
} 

static err_type wait_test1_fn(struct _awoke_wait_ev *ev)
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

err_type awoke_wait_test(int argc, char *argv[])
{
	log_debug("awoke_wait_test");
	waitev_test_t t;
	t.count = 0;
	awoke_wait_ev *ev = awoke_waitev_create("wait-test", 5, 
		WAIT_EV_F_SLEEP, wait_test1_fn, NULL, (void *)&t);
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
	int rc;
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
			int rc = read(event->fd, &r_header, sizeof(r_header));
			if ((event->fd == wk->pipe_channel.ch_r) && rc) {
				pipech_msg_header_dump(&r_header);
			}
		}
	}

	rc = write(t->pipe_channel.ch_w, &w_header, sizeof(w_header));
	log_debug("worker %s send msg(len:%d) to main", wk->name, rc);

	return et_ok;
}

static err_type benchmark_event_channel_test(int argc, char *argv[])
{
	int rc;
	pipech_msg_header w_header;
	pipech_msg_header r_header;
	err_type ret;
	awoke_event *event;
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
					rc = write(wk->pipe_channel.ch_w, (char *)&w_header, sizeof(w_header));
					log_debug("main send msg(len:%d) to worker %s", wk->name, rc);
				}
			}
		}
	}

	return et_ok;
}

static err_type benchmark_test_work(awoke_worker *wk)
{
	log_debug("run");
	return et_ok;
}

err_type benchmark_worker_test(int argc, char *argv[])
{
	awoke_worker *wk = awoke_worker_create("worker-test", 2, WORKER_FEAT_PERIODICITY|WORKER_FEAT_SUSPEND,
								  		   benchmark_test_work, NULL);
	awoke_worker_start(wk);

	sleep(10);

	awoke_worker_stop(wk);
	awoke_worker_destroy(wk);

	return et_ok;
}

static void *pthread_test_handle(void *arg)
{
	printf("pthread start!\n");

	sleep(2);

	return NULL;
} 

err_type benchmark_pthread_test(int argc, char *argv[])
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

#define init_bitflag()	{0x0000}	

static err_type benchmark_bitflag_test(int argc, char *argv[])
{
	typedef struct _bitflag {
		uint16_t f_active,
				 f_online,
				 f_protect,
				 f_multi;
	} bitflag;

	bitflag flag = init_bitflag();

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

	return et_ok;
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

	return et_ok;
}

static err_type benchmark_lock_test(int argc, char *argv[])
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

static err_type timer_worker_sched_end(awoke_tmwkr *twk)
{
	timer_worker_test_t *t = twk->data;
	t->x++;
	return et_ok;
}

static err_type benchmark_timer_worker_test(int argc, char *argv[])
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
	return ret;
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

static err_type benchmark_queue_zero_filter_test(int argc, char *argv[])
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

static err_type benchmark_vin_parse_test(int argc, char *argv[])
{
	int opt;
	char *vin = "LSG231413213";
	
	static const struct option long_opts[] = {
		{"vin",		required_argument,	NULL,	'v'},
        {NULL, 0, NULL, 0}
    };	

	while ((opt = getopt_long(argc, argv, "v:", long_opts, NULL)) != -1)
		{
			switch (opt)
			{
				case 'v':
					vin = optarg;
					break;
				
				default:
					break;
			}
		}

	vininfo vinfo;
	vin_parse(vin, &vinfo);
	vininfo_dump(&vinfo);
	return et_ok;
}

static err_type benchmark_http_request_test(int argc, char *argv[])
{
	int opt;
	err_type ret;
	char *method = "GET"; 
	char *body = "xxx";
	char *protocol = "HTTP/1.1"; 
	char *uri = "www.baidu.com";
	char *header_authorization = NULL;
	char *header_content_type = NULL;
	char *header_accept = NULL;
	char *header_accept_encoding = NULL;
	char *header_connection = "close";
	char *header_user_agent = NULL;
	http_header headers[HTTP_HEADER_SIZEOF];
	struct _http_connect connect_keep, *conn = NULL;
	struct _http_response response;
	
	static const struct option long_opts[] = {
		{"method",		required_argument,	NULL,	'm'},
		{"protocol",	required_argument,	NULL,	'p'},
        {"uri",			required_argument,	NULL,   'u'},
        {"body",  		required_argument,  NULL,   'b'},
        {"Content-Type",  	required_argument,  NULL,   arg_http_content_type},
        {"Authorization",  	required_argument,  NULL,   arg_http_authorization},
        {"Accept",  	required_argument,  NULL,   arg_http_accept},
        {"Connection",  required_argument,  NULL,   arg_http_connection},
        {"User-Agent",  required_argument,  NULL,   arg_http_user_agent},
        {"connect-export",  required_argument,  NULL,   arg_http_connect_keep},
        {NULL, 0, NULL, 0}
    };

	while ((opt = getopt_long(argc, argv, "u:m:p:b:?h", long_opts, NULL)) != -1)
    {
        switch (opt)
        {
        	case 'u':
				uri = optarg;
				break;

			case 'm':
				method = optarg;
				break;

			case 'p':
				protocol = optarg;
				break;				

        	case 'b':
				body = optarg;
				break;

			case arg_http_content_type:
				header_content_type = optarg;
				break;
				
			case arg_http_authorization:
				header_authorization = optarg;
				break;

			case arg_http_accept:
				header_accept = optarg;
				break;

			case arg_http_connection:
				header_connection = optarg;				
				break;

			case arg_http_user_agent:
				header_user_agent = optarg;
				break;

			case arg_http_connect_keep:
				conn = &connect_keep;
				break;
			
			case '?':
			case 'h':
				http_requset_usage(AWOKE_EXIT_SUCCESS);
			default:
				break;
        }
    }

	http_header_init(headers);
	http_header_set(headers, HTTP_HEADER_CONTENT_TYPE, header_content_type);
	http_header_set(headers, HTTP_HEADER_AUTHORIZATION, header_authorization);
	http_header_set(headers, HTTP_HEADER_ACCEPT, header_accept);
	http_header_set(headers, HTTP_HEADER_ACCEPT_ENCODING, header_accept_encoding);
	http_header_set(headers, HTTP_HEADER_CONNECTION, header_connection);
	http_header_set(headers, HTTP_HEADER_USER_AGENT, header_user_agent);

	/*
	 * <(debug): memory test, check if there are some mem not free>
	 *
	int loop = 1000;
	
	while (--loop) {
		if (!(loop%10)) log_warn("%d", loop);
		http_response_init(&response);
		ret = http_request(uri, method, protocol, body, headers, conn, &response);
		if (ret != et_ok) {
			log_err("http post request error, ret %d", ret);
			http_response_clean(&response);
			return ret;
		}

		http_response_clean(&response);
		usleep(100000);
		//sleep(1);
	}*/

	
	ret = http_request(uri, method, protocol, body, headers, conn, &response);
	if (ret != et_ok) {
		log_err("http post request error, ret %d", ret);
		return ret;
	}

	http_response_clean(&response);

	return et_ok;
}

static err_type benchmark_sscanf_test(int argc, char *argv[])
{
    /*
	int length;
	char *str = "Content-Length: 14615\r\ndsadsadsadasdsadasd";

	sscanf(str, "Content-Length: %d%*s", &length);
	log_debug("length:%d", length);
    */

    int raw = 66534;
    uint16_t volt;
    uint16_t data;

    volt = raw;
    log_debug("volt:%d", volt);
    data = htons(volt/10);
    log_debug("data:%d", data);

	return et_ok;
}

static err_type benchmark_buffchunk_test(int argc, char *argv[])
{
	//int i;
	//awoke_buffchunk *chunk;
	awoke_buffchunk_pool *pool;

	/*
	for (i=0; i<1000000; i++) {
		if (!(i%10)) log_warn("%d", i);
		chunk = awoke_buffchunk_create(1024);
		chunk->length = sprintf(chunk->p, "Hello world");
	
		pool = awoke_buffchunk_pool_create(8092);
		awoke_buffchunk_pool_dump(pool);

		awoke_buffchunk_pool_chunkadd(pool, chunk);
		//awoke_buffchunk_pool_dump(&pool);

		usleep(100);

		awoke_buffchunk_pool_chunkadd(pool, chunk);
		//awoke_buffchunk_pool_dump(&pool);

		awoke_buffchunk_pool_free(&pool);
		usleep(200);
	}*/

	pool = awoke_buffchunk_pool_create(8092);
	if (!pool) {
		log_err("bcpool create error");
		return et_nomem;
	}

	awoke_buffchunk *chunk1 = awoke_buffchunk_create(32);
	chunk1->length = sprintf(chunk1->p, "Hello world");
	awoke_buffchunk_dump(chunk1);

	awoke_buffchunk *chunk2 = awoke_buffchunk_create(32);
	chunk2->length = sprintf(chunk2->p, "Extend");
	awoke_buffchunk_dump(chunk2);

	awoke_buffchunk_pool_chunkadd(pool, chunk1);
	awoke_buffchunk_pool_chunkadd(pool, chunk2);

	awoke_buffchunk_pool_dump(pool);

	awoke_buffchunk *chunk3 = awoke_buffchunk_pool2chunk(pool);
	if (!chunk3) {
		log_err("pool to chunk error");
	}

	if (chunk3) awoke_buffchunk_dump(chunk3);
	log_debug("chunk3:%.*s", chunk3->length, chunk3->p);

	awoke_buffchunk_pool_free(&pool);
	if (chunk3) awoke_buffchunk_free(&chunk3);
	
	return et_ok;
}

static err_type benchmark_valist_test(int argc, char *argv[])
{
	log_debug("hello world");
	return et_ok;
}

#define mask_offset(n)	(0x00000001 << (n))
#define mask_test	mask_offset(0)
#define mask_drv	mask_offset(1)
#define mask_app	mask_offset(2)
#define mask_sys	mask_offset(3)

static err_type benchmark_time_zero_test(int argc, char *argv[])
{
    time_t now, zero;
    struct tm *tm;
    uint64_t us;
    time(&now);
    struct timeval tmus;
    gettimeofday(&tmus, NULL);
    us = tmus.tv_sec*1000000 + tmus.tv_usec;
    tm = gmtime(&now);
    tm->tm_sec = 0;
    tm->tm_min = 0;
    tm->tm_hour = 0;
    zero = mktime(tm);
    log_debug("now:%d zero:%d", now, zero);
    log_debug("mask_test:0x%x, 0x%x, 0x%x, 0x%x", mask_test, mask_drv, mask_app, mask_sys);
    log_debug("us:%ld", us);
    return et_ok;
}

static double gpsdata_test_abs(double x)
{
    if (x < 0) {
        x = x*(-1);
    }

    return x;
}

static err_type benchmark_gpsdata_test(int argc, char *argv[])
{
    uint16_t redius;
    double lat, lon;
    double new_lat, new_lon;
    double lat_diff, lon_diff;

#define GPSDATA_TEST_LAT_DISTANCE   111000
#define GPSDATA_TEST_LON_DISTANCE   92000

    lat = 30;
    lon = 40;
    new_lat = 32;
    new_lon = 42;
    redius = 100;

    lat_diff = gpsdata_test_abs(new_lat-lat);
    lon_diff = gpsdata_test_abs(new_lon-lon);

    log_debug("lat:%d lon:%d", lat, lon);
    log_debug("new_lat:%d new_lon:%d", new_lat, new_lon);
    log_debug("lat_diff:%d lon_diff:%d", lat_diff, lon_diff);

    log_debug("lat:%d lon:%d", (uint32_t)lat, (uint32_t)lon);
    log_debug("new_lat:%d new_lon:%d", (uint32_t)new_lat, (uint32_t)new_lon);
    log_debug("lat_diff:%d lon_diff:%d", (uint32_t)lat_diff, (uint32_t)lon_diff);   

    if ((lat_diff*GPSDATA_TEST_LAT_DISTANCE > redius)) {
        log_debug("lat redius");
    }

    if ((lon_diff*GPSDATA_TEST_LON_DISTANCE > redius)) {
        log_debug("lon redius");
    }

    return et_ok;
}

static void queue_dump(awoke_queue *q)
{
    int *p;
    awoke_queue_foreach(q, p, int) {
        log_debug("%d", *p);
    }
}

static err_type benchmark_queue_test(int argc, char *argv[])
{
    int x, a, b, c, d, e, f, g, h, i, j, *p, u;
    awoke_queue *q = awoke_queue_create(sizeof(int), 10, 
        AWOKE_QUEUE_F_IN|AWOKE_QUEUE_F_RB);

    a = 1;
    b = 2;
    c = 3;
    d = 4;
    e = 5;
    f = 6;
    g = 7;
    h = 8;
    i = 9;
    j = 10;
    x = 100;

    awoke_queue_enq(q, &a);
    awoke_queue_enq(q, &b);
    awoke_queue_enq(q, &c);
    awoke_queue_enq(q, &d);
    awoke_queue_enq(q, &e);
    awoke_queue_enq(q, &f);
    awoke_queue_enq(q, &g);
    awoke_queue_enq(q, &h);
    awoke_queue_enq(q, &i);
    awoke_queue_enq(q, &j);

    awoke_queue_foreach(q, p, int) {
        log_debug("%d", *p);
    }

    log_debug("---");

    int size = awoke_queue_size(q);

    for (i=0; i<size; i++) {
        awoke_queue_get(q, i, &u);
        log_debug("%d", u);
    }

    log_debug("i:%d ---", i);

    for (i=0; i<size; i++) {
        awoke_queue_deq(q, &u);
        log_debug("%d", u);
    }

    log_debug("%d---", i);
}

void private_info(int linenr, char *buff, int len, awoke_minpq *q)
{
    int i;
    int x, p;
    build_ptr bp = build_ptr_init(buff, len);

    build_ptr_string(bp, "value:   ");
    for (i=1; i<=MinPQ.size(q); i++) {
        MinPQ.get(q, &x, &p, i);
        build_ptr_format(bp, "%3d", x);
    }
}


/*
 * general mode test
 * custom define comparator
 * resize
 */

typedef struct _benchmark_minpq_struct {
    int p;
    int data;
} bmms;

void bmms_info_prior_handle(awoke_minpq *q, int width, char *buff, int len)
{
    int i;
    int p;
    bmms s;
	char *space = " ";
    
    build_ptr bp = build_ptr_init(buff, len);

    build_ptr_format(bp, "prior:%*s", width, space);
    for (i=1; i<=MinPQ.size(q); i++) {
        MinPQ.get(q, &s, &p, i);
        build_ptr_format(bp, "%*d", width, s.p);
    }
}

void bmms_info_value_handle(awoke_minpq *q, int width, char *buff, int len)
{
    int i;
    int p;
    bmms s;
	char *space = " ";
	
    build_ptr bp = build_ptr_init(buff, len);

    build_ptr_format(bp, "value:%*s", width, space);
    for (i=1; i<=MinPQ.size(q); i++) {
        MinPQ.get(q, &s, &p, i);
        build_ptr_format(bp, "%*d", width, s.data);
    }
}

static void bmms_init(bmms *s, int p, int data)
{
    s->p = p;
    s->data = data;
}

static bool bmms_comparator(void *s1, void *s2)
{
    bmms *b1 = (bmms *)s1;
    bmms *b2 = (bmms *)s2;

    return (b1->p > b2->p);
}

static err_type benchmark_minpq_custom_test(int argc, char *argv[])
{
    int p;
    bmms a, b, c, d, e, f, g, h, i, j, k, x;

    awoke_minpq *q = MinPQ.create(sizeof(bmms), 0, bmms_comparator, 0x0);
    MinPQ.set_info_handle(q, 5, bmms_info_prior_handle, bmms_info_value_handle);

    bmms_init(&a, 3, 1);
    bmms_init(&b, 3, 2);
    bmms_init(&c, 8, 3);
    bmms_init(&d, 2, 4);
    bmms_init(&e, 6, 5);
    bmms_init(&f, 13, 6);
    bmms_init(&g, 7, 7);
    bmms_init(&h, 9, 8);
    bmms_init(&i, 19, 9);
    bmms_init(&j, 5, 10);
    bmms_init(&k, 7, 11);

    MinPQ.insert(q, &a, 0);
    //MinPQ.info(q);
    MinPQ.insert(q, &b, 0);
    //MinPQ.info(q);
    MinPQ.insert(q, &c, 0);
    //MinPQ.info(q);
    MinPQ.insert(q, &d, 0);
    //MinPQ.info(q);
    MinPQ.insert(q, &e, 0);
    //MinPQ.info(q);
    MinPQ.insert(q, &f, 0);
    //MinPQ.info(q);
    MinPQ.insert(q, &g, 0);
    //MinPQ.info(q);
    MinPQ.insert(q, &h, 0);
    //MinPQ.info(q);
    MinPQ.insert(q, &i, 0);
    //MinPQ.info(q);
    MinPQ.insert(q, &j, 0);
    //MinPQ.info(q);
    MinPQ.insert(q, &k, 0);
    MinPQ.info(q);

	awoke_minpq_del(q, &x, &p, 5);
	log_debug("delmin:%d %d", x.p, x.data);
	MinPQ.info(q);

    MinPQ.delmin(q, &x, &p);
    log_debug("delmin:%d %d", x.p, x.data);
    MinPQ.delmin(q, &x, &p);
    log_debug("delmin:%d %d", x.p, x.data);
    MinPQ.delmin(q, &x, &p);
    log_debug("delmin:%d %d", x.p, x.data);

    MinPQ.delmin(q, &x, &p);
    log_debug("delmin:%d %d", x.p, x.data);
    MinPQ.delmin(q, &x, &p);
    log_debug("delmin:%d %d", x.p, x.data);
    MinPQ.delmin(q, &x, &p);
    log_debug("delmin:%d %d", x.p, x.data);

    MinPQ.delmin(q, &x, &p);
    log_debug("delmin:%d %d", x.p, x.data);
    MinPQ.delmin(q, &x, &p);
    log_debug("delmin:%d %d", x.p, x.data);
    MinPQ.delmin(q, &x, &p);
    log_debug("delmin:%d %d", x.p, x.data);

    MinPQ.info(q);

    return et_ok;
}

static err_type benchmark_minpq_test(int argc, char *argv[])
{
    int a, b, c, d, e, f, g, h, i, j, k, x, p;

    awoke_minpq *q = MinPQ.create(sizeof(int), 20, NULL, 0x0);
    MinPQ.info(q);

    a = 1;
    b = 2;
    c = 3;
    d = 4;
    e = 5;
    f = 6;
    g = 7;
    h = 8;

    MinPQ.insert(q, &a, 2);
    MinPQ.info(q);

    MinPQ.insert(q, &b, 3);
    MinPQ.insert(q, &b, 13);
    MinPQ.insert(q, &d, 11);
    MinPQ.insert(q, &h, 9);
    MinPQ.insert(q, &d, 17);
    MinPQ.insert(q, &c, 8);
    MinPQ.insert(q, &f, 5);
    MinPQ.insert(q, &f, 3);
    MinPQ.info(q);

    MinPQ.delmin(q, &x, &p);
    log_debug("delmin:%d %d", p, x);
    MinPQ.delmin(q, &x, &p);
    log_debug("delmin:%d %d", p, x);
    MinPQ.delmin(q, &x, &p);
    log_debug("delmin:%d %d", p, x);
    MinPQ.info(q);

    MinPQ.delmin(q, &x, &p);
    log_debug("delmin:%d %d", p, x);
    MinPQ.delmin(q, &x, &p);
    log_debug("delmin:%d %d", p, x);
    MinPQ.delmin(q, &x, &p);
    log_debug("delmin:%d %d", p, x);
    MinPQ.info(q);

    return et_ok;
}

void fifo_test_value_handle(awoke_fifo *f, int width, char *buff, int len)
{
    int i;
    int p;
    int d;
    
    build_ptr bp = build_ptr_init(buff, len);

    build_ptr_string(bp, "value:");
    for (i=0; i<awoke_fifo_size(f); i++) {
        awoke_fifo_get(f, &d, i);
        build_ptr_format(bp, "%*d", width, d);
    }
}

static err_type benchmark_fifo_test(int argc, char *argv[])
{
	int x, a, b, c, d, e, f, g, h, i, j, k;

	awoke_fifo *fifo = awoke_fifo_create(sizeof(int), 8, AWOKE_FIFO_F_RBK);
	awoke_fifo_dumpinfo_set(fifo, 4, fifo_test_value_handle, NULL);

	a = 1;
    b = 2;
    c = 3;
    d = 4;
    e = 5;
    f = 6;
    g = 7;
    h = 8;
    i = 9;
    j = 10;
    x = 100;

	awoke_fifo_dump(fifo);

	awoke_fifo_enqueue(fifo, &a);
	awoke_fifo_enqueue(fifo, &b);
	awoke_fifo_enqueue(fifo, &c);
	awoke_fifo_enqueue(fifo, &d);
	awoke_fifo_enqueue(fifo, &e);
	awoke_fifo_enqueue(fifo, &f);
	awoke_fifo_enqueue(fifo, &g);
	awoke_fifo_enqueue(fifo, &h);
	awoke_fifo_enqueue(fifo, &i);
	awoke_fifo_enqueue(fifo, &j);

	awoke_fifo_dump(fifo);

	awoke_fifo_dequeue(fifo, &x);
	awoke_fifo_dequeue(fifo, &x);
	awoke_fifo_dequeue(fifo, &x);
	
	awoke_fifo_dump(fifo);
	
	awoke_fifo_dequeue(fifo, &x);
	awoke_fifo_dequeue(fifo, &x);
	awoke_fifo_dequeue(fifo, &x);

	awoke_fifo_dump(fifo);
	
	awoke_fifo_dequeue(fifo, &x);
	awoke_fifo_dequeue(fifo, &x);

	awoke_fifo_dump(fifo);
}

static err_type benchmark_md5_test(int argc, char *argv[])
{
	char *p;
	char buf[1024];
	char md5in[32];
	char md5out[16];
	char imeihex[8];
	char imeibcd[16];
	char *imeistr = "0123456789012347";
	char *slatstr = "secret";
	uint8_t version = 0;
	uint8_t slat_byte[8] = {0x0};
	uint8_t imei_byte[8] = {0x0};
	uint8_t *pos = buf;
	uint8_t *plast = (uint8_t *)slatstr;

	awoke_string_str2bcd(imei_byte, imeistr, strlen(imeistr));
	awoke_string_bcd2str(imeibcd, imei_byte, 8);
	log_trace("imei:%s", imeistr);
	awoke_hexdump_trace(imei_byte, 8);
	log_trace("imeibcd:%s", imeibcd);
	
	memcpy(slat_byte, plast, 6);

	p = md5in;
	memcpy(p, imei_byte, 8);
	p += 8;
	memcpy(p, slat_byte, 8);

	//log_debug("MD5in:");
	//awoke_hexdump_trace(md5in, 16);
	md5(md5in, 16, md5out);

	pkg_push_byte(version, pos);
	pkg_push_bytes(imei_byte, pos, 8);
	pkg_push_bytes(md5out, pos, 16);

	log_trace("version:%d", version);
	log_trace("imei:%s", imeistr);
	log_trace("slat:%s", slatstr);
	log_debug("MD5:");
	awoke_hexdump_trace(md5out, 16);
	log_debug("LoginRequest:");
	awoke_hexdump_trace(buf, 25);

	return et_ok;
}

static err_type benchmark_socket_poll_test(int argc, char *argv[])
{
	int fd_server, fd_client;
	struct sockaddr_in servaddr, clientaddr;
	
	fd_server = socket(PF_INET, SOCK_STREAM, 0);
	log_debug("serverfd:%d", fd_server);
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = PF_INET;
	servaddr.sin_port = htons(8008);
	inet_pton(PF_INET, "0.0.0.0", &servaddr.sin_addr);

	bind(fd_server, (struct sockaddr*)&servaddr, sizeof(servaddr));
	log_debug("bind");
	listen(fd_server, 5);
	log_debug("listen");

	fd_client = socket(PF_INET, SOCK_STREAM, 0);
	log_debug("clientfd:%d", fd_client);
	connect(fd_client, (struct sockaddr*)&servaddr, sizeof(servaddr));
	

	/*
	struct pollfd fds[2];

	fds[0].fd = fd_server;
	fds[0].events = POLLIN;
	fds[0].revents = 0;

	fds[1].fd = fd_client;
	fds[1].events = POLLIN | POLLOUT;
	fds[1].revents = 0;

	int ret = poll(fds, 2, 1000);
	log_debug("ret:%d", ret);
	log_debug("fd[0]:%d %d %d", fds[0].fd, fds[0].events, fds[0].revents);
	log_debug("fd[1]:%d %d %d", fds[1].fd, fds[1].events, fds[1].revents);

	fds[0].events |= POLLOUT;

	ret = poll(fds, 2, 1000);
	log_debug("fd[0]:%d %d %d", fds[0].fd, fds[0].events, fds[0].revents);
	log_debug("fd[1]:%d %d %d", fds[1].fd, fds[1].events, fds[1].revents);
	*/

	fd_set readfds, writefds, errfds;
	int max_fd = 0;
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&errfds);

	FD_SET(fd_server, &readfds);
	FD_SET(fd_client, &readfds);

	select(max_fd + 1, &readfds, &writefds, &errfds, 1000);
	if (FD_ISSET(fd_server, &readfds)) {
		log_debug("server read");
	}
	if (FD_ISSET(fd_client, &readfds)) {
		log_debug("client read");
	}

	if (FD_ISSET(fd_server, &writefds)) {
		log_debug("server write");
	}
	if (FD_ISSET(fd_client, &writefds)) {
		log_debug("client write");
	}

	FD_SET(fd_server, &writefds);
	select(max_fd + 1, &readfds, &writefds, &errfds, 1000);
	if (FD_ISSET(fd_server, &readfds)) {
		log_debug("server read");
	}
	if (FD_ISSET(fd_client, &readfds)) {
		log_debug("client read");
	}

	if (FD_ISSET(fd_server, &writefds)) {
		log_debug("server write");
	}
	if (FD_ISSET(fd_client, &writefds)) {
		log_debug("client write");
	}

	close(fd_client);
	close(fd_server);
}

typedef struct {
	int d;
	awoke_list _head;
} listtest;

static void listtest_dump(awoke_list *xlist)
{
	listtest *p;
	log_debug("");
	log_debug("xlist dump");
	log_debug("----------------");
	list_for_each_entry(p, xlist, _head) {
		log_debug("%d", p->d);
	}
	log_debug("----------------");
	log_debug("");
}

static void listtest_insert(listtest *x, awoke_list *xlist)
{	
	listtest *p;

	if (list_empty(xlist)) {
		return list_append(&x->_head, xlist);
	}
	
	list_for_each_entry(p, xlist, _head) {
		if (p->d > x->d) {
			return list_append(&x->_head, &p->_head);
		}
	}

	list_append(&x->_head, xlist);
}

static err_type benchmark_list_test(int argc, char *argv[])
{
	listtest a, b, c, d, e, f, g, h;
	awoke_list xlist;

	list_init(&xlist);

	a.d = 1;
	b.d = 2;
	c.d = 3;
	d.d = 4;
	e.d = 5;
	f.d = 6;
	g.d = 7;
	h.d = 8;

	listtest_insert(&d, &xlist);
	listtest_insert(&f, &xlist);
	listtest_insert(&e, &xlist);
	listtest_insert(&a, &xlist);
	listtest_insert(&h, &xlist);

	listtest_dump(&xlist);

	listtest_insert(&g, &xlist);
	listtest_insert(&b, &xlist);

	listtest_dump(&xlist);
	
	return et_ok;
}

typedef struct {
	int id;
	char name[32];
	awoke_list _head;
} logfile_cache;

typedef struct {
	char currname[32];
	int currsize;
	int maxsize;
	int cachenum;
	int cachemax;
	awoke_list logfile_caches;
} logfile_struct;

static logfile_struct logfilem;

static bool logfile_full(logfile_struct *m)
{
	int size;

	size = m->currsize;

	if (size >= m->maxsize) {
		log_debug("current file full");
		return TRUE;
	}

	return FALSE;
}

static void logfile_curr2cache(logfile_struct *m)
{
	char rn[32] = {'\0'};
	logfile_cache *c;
	logfile_cache *newc;
		
	if (m->cachenum < m->cachemax) {
		list_for_each_entry(c, &m->logfile_caches, _head) {
			memset(rn, 0x0, 32);
			c->id++;
			sprintf(rn, "xxx.log.%d", c->id);
			log_trace("file:%s rename to %s", c->name, rn);
			sprintf(c->name, rn);
		}

		newc = mem_alloc_z(sizeof(logfile_cache));
		newc->id = 1;
		sprintf(newc->name, "xxx.log.%d", newc->id);
		log_trace("name:%s", newc->name);
		list_append(&newc->_head, &m->logfile_caches);
		log_trace("file %d:%s add to cache", newc->id, newc->name);
		m->cachenum++;
		m->currsize = 0;
		return;
	}

	log_trace("cache full");

	c = list_entry_first(&m->logfile_caches, logfile_cache, _head);
	log_trace("remove cache %d:%s", c->id, c->name);
	list_unlink(&c->_head);
	mem_free(c);
	
	list_for_each_entry(c, &m->logfile_caches, _head) {
		memset(rn, 0x0, 32);
		c->id++;
		sprintf(rn, "xxx.log.%d", c->id);
		log_trace("file:%s rename to %s", c->name, rn);
		sprintf(c->name, rn);
	}

	newc = mem_alloc_z(sizeof(logfile_cache));
	newc->id = 1;
	sprintf(newc->name, "xxx.log.%d", newc->id);
	list_append(&newc->_head, &m->logfile_caches);
	log_trace("file %d:%s add to cache", newc->id, newc->name);
	m->currsize = 0;
}

static void logfile_cache_dump(logfile_struct *m)
{
	logfile_cache *c;
	
	log_debug("");
	log_debug("cache dump");
	log_debug("----------------");

	list_for_each_entry(c, &m->logfile_caches, _head) {
		log_debug("id:%d name:%s", c->id, c->name);
	}

	log_debug("----------------");
	log_debug("");
}

static void *logfile_cache_write_message(void *ctx)
{
	log_trace("write message");

	if (logfile_full(&logfilem)) {
		logfile_curr2cache(&logfilem);
	}

	logfilem.currsize += 553;
	log_trace("currsize:%d", logfilem.currsize);

	logfile_cache_dump(&logfilem);
}

static err_type benchmark_logfile_cache_test(int argc, char *argv[])
{
	logfilem.cachemax = 3;
	logfilem.cachenum = 0;
	logfilem.maxsize = 2048;
	logfilem.currsize = 0;
	list_init(&logfilem.logfile_caches);

	sprintf(logfilem.currname, "xxx.log");

	log_info("");
	log_info("logfile cache test start");
	log_info("");

	awoke_worker *wk = awoke_worker_create("logfile-cache-test", 1, 
										   WORKER_FEAT_PERIODICITY|WORKER_FEAT_SUSPEND,
								  		   logfile_cache_write_message, NULL);
	awoke_worker_start(wk);

	sleep(30);

	awoke_worker_stop(wk);
	awoke_worker_destroy(wk);

	log_info("");
	log_info("logfile cache test end");
	log_info("");

	return et_ok;	
}

static err_type benchmark_log_color_test(int argc, char *argv[])
{
	log_burst("log color test");
	log_trace("log color test");
	log_debug("log color test");
	log_info("log color test");
	log_notice("log color test");
	log_err("log color test");
	log_warn("log color test");
	log_bug("log color test");
}

static void *log_cache_work(void *ctx)
{
	bk_debug("cache test");
}

static err_type benchmark_log_cache_test(int argc, char *argv[])
{
	awoke_worker *wk = awoke_worker_create("log-cache", 200,
										   WORKER_FEAT_PERIODICITY|WORKER_FEAT_SUSPEND|WORKER_FEAT_TICK_MSEC,
								  		   log_cache_work, NULL);
	awoke_worker_start(wk);

	sleep(120);

	awoke_worker_stop(wk);
	awoke_worker_destroy(wk);
}

#define PACKED_ALIGN_BYTE __attribute__ ((packed,aligned(1)))

struct CameraConfig {

	uint32_t mark;

	uint16_t goal;
	uint16_t goal_min;
	uint16_t goal_max;
	
	uint16_t expo;
	uint16_t expo_min;
	uint16_t expo_max;

	uint16_t gain;
	uint16_t gain_min;
	uint16_t gain_max;

	uint8_t fps;
	uint8_t fps_min;
	uint8_t fps_max;

	uint8_t hinvs;
	uint8_t vinvs;

	uint8_t ae_frame;

	int32_t tec_target;
	uint32_t tecwork_freq;

	uint32_t ae_enable:1;
	uint32_t ae_expo_enable:1;
	uint32_t ae_gain_enable:1;
	uint32_t hdr_enable:1;
	uint32_t cst_enable:1;
	uint32_t dfg_enable:1;
	uint32_t nrd_enable:1;
	uint32_t ehc_enable:1;
	uint32_t shp_enable:1;
	uint32_t zoom_enable:1;
	uint32_t epc_testbit:1;
	uint32_t inversion_enable:1;
	uint32_t crossview_enable:1;
	uint32_t config_dump:1;
	uint32_t epc_info_dump:1;
	uint32_t tec_enable:1;
	uint32_t cltest_enable:1;
	uint32_t frtest_enable:1;
	uint32_t rsv01:16;

	uint16_t crosscp_x;
	uint16_t crosscp_y;

	//uint8_t checksum;
} PACKED_ALIGN_BYTE;

uint8_t config_checksum(uint8_t *buf, int len)
{
	int i;
	uint8_t sum = 0x0;

	for (i=0; i<(len-1); i++) {
		sum += buf[i];
		//bk_debug("buf:%d sum:%d", buf[i], sum);
	}

	return sum;
}

static err_type benchmark_cameraconfig_test(int argc, char *argv[])
{
	int fd, rc;
	char buffer[1024];
	uint8_t checksum;
	struct CameraConfig *cfg;

	fd = open("CameraConfig.bin", O_RDONLY);
	if (fd < 0) {
		bk_err("open file CameraConfig.bin error");
		return et_file_open;
	}

	rc = read(fd, buffer, 1024);
	bk_debug("read:%d", rc);
	close(fd);

	checksum = config_checksum(buffer, rc);
	bk_debug("checksum:0x%x", checksum);

	cfg = (struct CameraConfig *)buffer;
	awoke_hexdump_trace(buffer, rc);
	bk_debug("mark:0x%x", cfg->mark);
	bk_debug("goal:%d min:%d max:%d", cfg->goal, cfg->goal_min, cfg->goal_max);
	return et_ok;
}

static err_type benchmark_sensorconfig_test(int argc, char *argv)
{
	int fd, rc, i;
	char buffer[1024];
	uint8_t checksum, *pos;
	uint32_t mark;
	uint16_t addr;
	uint8_t value;

	fd = open("SensorConfig.bin", O_RDONLY);
	if (fd < 0) {
		bk_err("open file SensorConfig.bin error");
		return et_file_open;
	}

	rc = read(fd, buffer, 1024);
	bk_debug("read:%d", rc);
	close(fd);
	checksum = config_checksum(buffer, rc);
	bk_debug("checksum:0x%x", checksum);
	awoke_hexdump_trace(buffer, rc);

	pos = (uint8_t *)buffer;
	pkg_pull_dwrd(mark, pos);
	bk_debug("mark:0x%x", mark);

	for (i=0; i<50; i++) {
		pkg_pull_word(addr, pos);
		pkg_pull_byte(value, pos);
		bk_debug("addr:0x%x value:0x%x", addr, value);
	}

	return et_ok;
}

int main(int argc, char *argv[])
{
	int opt;
	char *app_id = "benchmark";
	int loglevel = LOG_TRACE;
	int logmode = LOG_TRACE;
	bencmark_func bmfn = NULL;

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
		{"http-request-test",	no_argument,		NULL,	arg_http_request_test},
		{"sscanf-test",			no_argument,		NULL,	arg_sscanf_test},
		{"buffchunk-test",      no_argument, 		NULL,	arg_buffchunk_test},
		{"valist-test",			no_argument,		NULL,	arg_valist_test},
        {"timezero-test",		no_argument,		NULL,	arg_time_zero},
        {"gpsdata-test",        no_argument,        NULL,   arg_gpsdata_test},
        {"queue-test",          no_argument,        NULL,   arg_queue_test},
        {"minpq-test",          no_argument,        NULL,   arg_minpq_test},
		{"fifo-test",  			no_argument, 		NULL,   arg_fifo_test},
		{"md5-test",			no_argument, 		NULL,	arg_md5_test},
		{"socket-poll-test",	no_argument,		NULL,	arg_socket_poll_test},
		{"list-test",			no_argument,		NULL,	arg_list_test},
		{"logfile-cache-test",	no_argument, 		NULL,	arg_logfile_cache_test},
		{"filecache-test",		no_argument, 		NULL,	arg_filecache_test},
		{"log-color-test",		no_argument,		NULL,	arg_log_color_test},
		{"log-cache-test",		no_argument,		NULL,	arg_log_cache_test},
		{"cameraconfig-test",   no_argument,        NULL,   arg_cameraconfig_test},
		{"sensorconfig-test",   no_argument,        NULL,   arg_sensorconfig_test},
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
				bmfn = benchmark_vin_parse_test;
				break;

			case arg_http_request_test:
				bmfn = benchmark_http_request_test;
				goto run;

			case arg_sscanf_test:
				bmfn = benchmark_sscanf_test;
				break;

			case arg_buffchunk_test:
				bmfn = benchmark_buffchunk_test;
				break;

			case arg_valist_test:
				bmfn = benchmark_valist_test;
				break;
			
            case arg_time_zero:
                bmfn = benchmark_time_zero_test;
                break;

            case arg_gpsdata_test:
                bmfn = benchmark_gpsdata_test;
                break;

            case arg_queue_test:
                bmfn = benchmark_queue_test;
                break;

            case arg_minpq_test:
                bmfn = benchmark_minpq_custom_test;
                break;

			case arg_fifo_test:
				bmfn = benchmark_fifo_test;
				break;

			case arg_md5_test:
				bmfn = benchmark_md5_test;
				break;

			case arg_socket_poll_test:
				bmfn = benchmark_socket_poll_test;
				break;

			case arg_list_test:
				bmfn = benchmark_list_test;
				break;

			case arg_logfile_cache_test:
				bmfn = benchmark_logfile_cache_test;
				break;

			case arg_filecache_test:
				bmfn = benchmark_filecache_test;
				break;

			case arg_log_color_test:
				bmfn = benchmark_log_color_test;
				break;

			case arg_log_cache_test:
				bmfn = benchmark_log_cache_test;
				break;

			case arg_cameraconfig_test:
				bmfn = benchmark_cameraconfig_test;
				break;

			case arg_sensorconfig_test:
				bmfn = benchmark_sensorconfig_test;
				break;

            case '?':
            case 'h':
			case '-':
            default:
                usage(AWOKE_EXIT_SUCCESS);
        }
    }

run:
	awoke_log_init(loglevel, 0);
	bmfn(argc, argv);

	return 0;
}
