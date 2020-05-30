
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
	int length;
	char *str = "Content-Length: 14615\r\ndsadsadsadasdsadasd";

	sscanf(str, "Content-Length: %d%*s", &length);
	log_debug("length:%d", length);

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

static err_type benchmark_time_zero_test(int argc, char *argv[])
{
    time_t now, zero;
    struct tm *tm;
    
    time(&now);
    tm = gmtime(&now);
    tm->tm_sec = 0;
    tm->tm_min = 0;
    tm->tm_hour = 0;
    zero = mktime(tm);
    log_debug("now:%d zero:%d", now, zero);
    return et_ok;
}

static int fastlz_pkg(uint8_t *src, int num)
{
    typedef struct _fastlz_test_gpsinfo {
        uint32_t time;
        uint32_t lat;
        uint32_t lon;
        uint8_t speed;
        uint16_t alt;
        uint8_t angle;
    } fastlz_test_gpsinfo;

	fastlz_test_gpsinfo gpstable[] = {
		{1590719644, 3305864961, 4052158478, 0, 453, 0},
		{1590719646, 3305864963, 4052158463, 0, 454, 0},
		{1590719647, 3305864927, 4052158475, 0, 455, 0},
		{1590719649, 3305864935, 4052158466, 0, 455, 0},
		{1590719651, 3305864914, 4052158412, 0, 453, 0},
		{1590719652, 3305864982, 4052158432, 0, 453, 0},
		{1590719654, 3305864931, 4052158456, 0, 453, 0},
		{1590719656, 3305864949, 4052158413, 0, 453, 0},
		{1590719658, 3305864998, 4052158434, 0, 453, 0},
		{1590719661, 3305864927, 4052158432, 0, 453, 0},
		{1590719663, 3305864989, 4052158433, 0, 453, 0},
		{1590719665, 3305864972, 4052158466, 0, 453, 0},
		{1590719666, 3305864909, 4052158457, 0, 453, 0},
		{1590719667, 3305864906, 4052158456, 0, 453, 0},
		{1590719669, 3305864918, 4052158463, 0, 453, 0},
		{1590719671, 3305864908, 4052158445, 0, 453, 0},
		{1590719673, 3305864917, 4052158467, 0, 453, 0},
		{1590719674, 3305864958, 4052158475, 0, 453, 0},
		{1590719676, 3305864909, 4052158465, 0, 453, 0},
		{1590719678, 3305864933, 4052158454, 0, 453, 0},
		{1590719680, 3305864967, 4052158446, 0, 453, 0},
		{1590719682, 3305864985, 4052158486, 0, 453, 0},
		{1590719684, 3305864901, 4052158445, 0, 453, 0},
		{1590719686, 3305864923, 4052158447, 0, 453, 0},
		{1590719688, 3305864921, 4052158456, 0, 453, 0},
		{1590719690, 3305864935, 4052158456, 0, 453, 0},
		{1590719692, 3305864956, 4052158423, 0, 453, 0},
		{1590719694, 3305864977, 4052158456, 0, 453, 0},
		{1590719696, 3305864908, 4052158455, 0, 453, 0},
		{1590719697, 3305864976, 4052158498, 0, 453, 0},
		{1590719698, 3305864923, 4052158475, 0, 453, 0},
		{1590719701, 3305864987, 4052158457, 0, 453, 0},
		{1590719703, 3305864985, 4052158443, 0, 453, 0},
		{1590719705, 3305864976, 4052158457, 0, 453, 0},
		{1590719707, 3305864909, 4052158446, 0, 453, 0},
		{1590719709, 3305864968, 4052158454, 0, 453, 0},
		{1590719711, 3305864932, 4052158446, 0, 453, 0},
		{1590719713, 3305864933, 4052158454, 0, 453, 0},
		{1590719715, 3305864944, 4052158498, 0, 453, 0},
		{1590719718, 3305864955, 4052158425, 0, 453, 0},
                                                     
		{1590719719, 3305864955, 4052158425, 0, 453, 0},
		{1590719721, 3305864931, 4052158425, 0, 453, 0},
		{1590719723, 3305864956, 4052158425, 0, 453, 0},
		{1590719724, 3305864954, 4052158425, 0, 453, 0},
		{1590719725, 3305864965, 4052158425, 0, 453, 0},
		{1590719727, 3305864973, 4052158425, 0, 453, 0},
		{1590719719, 3305864957, 4052158425, 0, 453, 0},
		{1590719721, 3305864948, 4052158422, 0, 453, 0},
		{1590719723, 3305864975, 4052158441, 0, 453, 0},
		{1590719725, 3305864919, 4052158435, 0, 453, 0},
		{1590719727, 3305864986, 4052158466, 0, 453, 0},
		{1590719729, 3305864965, 4052158479, 0, 453, 0},
		{1590719731, 3305864985, 4052158465, 0, 453, 0},
		{1590719733, 3305864983, 4052158458, 0, 453, 0},
		{1590719735, 3305864981, 4052158495, 0, 453, 0},
		{1590719738, 3305864902, 4052158475, 0, 453, 0},
		{1590719740, 3305864992, 4052158448, 0, 453, 0},
		{1590719742, 3305864958, 4052158405, 0, 453, 0},
		{1590719744, 3305864987, 4052158496, 0, 453, 0},
		{1590719746, 3305864931, 4052158448, 0, 453, 0},
		{1590719748, 3305864966, 4052158447, 0, 453, 0},
		{1590719752, 3305864967, 4052158449, 0, 453, 0},
		{1590719754, 3305864989, 4052158445, 0, 453, 0},
		{1590719758, 3305864908, 4052158431, 0, 453, 0},
		{1590719763, 3305864932, 4052158435, 0, 453, 0},
		{1590719766, 3305864967, 4052158452, 0, 453, 0},
		{1590719768, 3305864984, 4052158456, 0, 453, 0},
		{1590719771, 3305864990, 4052158498, 0, 453, 0},
		{1590719774, 3305864945, 4052158445, 0, 453, 0},
		{1590719777, 3305864987, 4052158479, 0, 453, 0},
		{1590719779, 3305864956, 4052158402, 0, 453, 0},
		{1590719782, 3305864987, 4052158441, 0, 453, 0},
		{1590719784, 3305864945, 4052158473, 0, 453, 0},
		{1590719788, 3305864967, 4052158461, 0, 453, 0},
		{1590719791, 3305864996, 4052158461, 0, 453, 0},
		{1590719794, 3305864945, 4052158467, 0, 453, 0},
		{1590719795, 3305864967, 4052158484, 0, 453, 0},
		{1590719799, 3305864954, 4052158462, 0, 453, 0},
		{1590719811, 3305864956, 4052158494, 0, 453, 0},
		{1590719815, 3305864945, 4052158472, 0, 453, 0},		
	};

	int i;

	uint8_t *pos = src;
	#if 0
    uint16_t cmd = 0x0a02;
    uint8_t devmode = 0x01;
    uint8_t hoststate = 0x02;
    uint8_t motionstate = 0x02;
    uint16_t devvolt = 0x1a2;
    uint8_t voltper = 100;
    uint8_t devtemp = 0x87;
    uint8_t hostpos = 1;
	uint8_t reserve = 0x0;
    uint32_t celltime = 0x00034157;
    uint16_t mcc = 0x0460;
    uint8_t mnc = 0x11;
    uint8_t act = 0x0;
    uint16_t tac = 0x4b00;
    uint32_t cellid = 0x9356a12;
    uint16_t earfcn = 0xe68;
    uint16_t pci = 0x1dd;
    uint8_t rsrp = 0x56;
    uint8_t rsrq = 0xf4;
    uint8_t rssi = 0xfe;
    uint16_t snr = 0x3f2;
    uint8_t cellnum = 5;
    uint16_t neighbor1_earfcn = 0xe68;
    uint16_t neighbor1_pci = 0xa1;
    uint8_t neighbor1_rsrp = 0x62;
    uint8_t neighbor1_rsrq = 0xf3;
    uint8_t neighbor1_rssi = 0xf2;
    uint16_t neighbor1_snr = 0x416;
   
	/* pkg cmd */
	cmd = htons(cmd);
	pkg_push_word(cmd, pos);

	/* device mode */
	pkg_push_byte(devmode, pos);

	/* host state */
	pkg_push_byte(hoststate, pos);

	/* motion state */
	pkg_push_byte(motionstate, pos);

	/* device volt */
	devvolt = htons(devvolt);
	pkg_push_word(devvolt, pos);

	/* volt percent */
	pkg_push_byte(voltper, pos);

	/* tempurate */
	pkg_push_byte(devtemp, pos);

	/* hostpos */
	pkg_push_byte(hostpos, pos);

	/* reserve */
	pkg_push_bytes(reserve, pos, 3);

	/* celltime */
	celltime = htonl(celltime);
	pkg_push_dwrd(celltime, pos);

	/* MCC */
	mcc = htons(mcc);
	pkg_push_word(mcc, pos);

	/* MNC */
	pkg_push_byte(mnc, pos);

	/* ACT */
	pkg_push_byte(act, pos);

	/* TAC */
	pkg_push_byte(tac, pos);

	/* cellid */
	cellid = htonl(cellid);
	pkg_push_dwrd(cellid, pos);

	/* earfcn */
	earfcn = htons(earfcn);
	pkg_push_word(earfcn, pos);

	/* PCI */
	pci = htons(pci);
	pkg_push_word(pci, pos);

	/* RSRP */
	pkg_push_byte(rsrp, pos);

	/* RSRQ */
	pkg_push_byte(rsrq, pos);

	/* RSSI */
	pkg_push_byte(rssi, pos);

	/* SNR */
	pkg_push_byte(snr, pos);

	/* NCellnum */
	pkg_push_byte(cellnum, pos);
	#endif

	uint8_t gpsnum = num;
	if (gpsnum == 1)
		gpsnum = 2;
	
	/* gpsnum */
	pkg_push_byte(gpsnum, pos);

	for (i=0; i<(gpsnum-1); i++) {

		fastlz_test_gpsinfo *gpsinfo = &gpstable[i];

		/* time */
		uint32_t time = htonl(gpsinfo->time);
		pkg_push_dwrd(time, pos);

		/* lat */
		uint32_t lat = htonl(gpsinfo->lat);
		pkg_push_dwrd(lat, pos);

		/* lon */
		uint32_t lon = htonl(gpsinfo->lon);
		pkg_push_dwrd(lon, pos);

		/* speed */
		uint8_t speed = gpsinfo->speed;
		pkg_push_byte(speed, pos);

		/* alt */
		uint8_t alt = gpsinfo->alt;
		pkg_push_byte(alt, pos);

		/* angle */
		uint16_t angle = htons(gpsinfo->angle);
		pkg_push_word(angle, pos);
	}

	return (pos-src);
}

static err_type fastlz_test(int number, int cmplevle, bool dump)
{
    bool check_ok = TRUE;
    int i, length;    
	int cmplens, dpmlens;

	int rate;
    uint8_t *src = mem_alloc_z(4096);
    uint8_t *cmp = mem_alloc_z(4096);
    uint8_t *dmp = mem_alloc_z(4096);

    /* int source data */
	length = fastlz_pkg(src, number);

    if (dump) {
        pkg_dump(src, length);
    }

    /* compress */
	cmplens = LZ4_compress_default(src, cmp, length, 4096);
	//cmplens = lzjb_compress(src, cmp, length, 4096);
    //cmplens = fastlz_compress_level(cmplevle, src, length, cmp);
    if (dump) {
        pkg_dump(cmp, cmplens);
    }

    /* decompress */
	dpmlens = LZ4_decompress_safe(cmp, dmp, cmplens, 4096);
	//dpmlens = lzjb_compress(cmp, dmp, cmplens, 4096);
    //dpmlens = fastlz_decompress(cmp, cmplens, dmp, length);
    if (dump) {
        pkg_dump(dmp, dpmlens);
    }

    /* check */
    for (i=0; i<(length-1); i++) {
        if (src[i] != dmp[i]) {
            check_ok = FALSE;
            break;
        }
    }

	rate = 100 - cmplens*100/length;

    mem_free(src);
    mem_free(cmp);
    mem_free(dmp);

	printf("%d\t%d\t%d\t  %d\t\t%d\t%d\r\n", 
		number, length, cmplens, dpmlens, check_ok, rate);
	
    return et_ok;
}

static err_type benchmark_fastlz_test(int argc, char *argv[])
{
    int i = 0;

    int length_table[] = {5, 10, 15, 20, 25, 30, 40, 60, 80};
    int length_table_size = array_size(length_table);

	printf("gpsnum\tsource\tcompress  decompress\tcheck\trate(\%)\r\n");

    for (i=0; i<(length_table_size-1); i++) {
        fastlz_test(length_table[i], 1, FALSE);
    }

    return et_ok;
}

int main(int argc, char *argv[])
{
	int opt;
	char *app_id = "benchmark";
	int loglevel = LOG_DBG;
	int logmode = LOG_TEST;
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
        {"fastlz-test",         no_argument,        NULL,   arg_fastlz_test},
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

            case arg_fastlz_test:
                bmfn = benchmark_fastlz_test;
                break;

            case '?':
            case 'h':
			case '-':
            default:
                usage(AWOKE_EXIT_SUCCESS);
        }
    }

run:
	log_set(app_id, logmode, loglevel);
	bmfn(argc, argv);

	return 0;
}
