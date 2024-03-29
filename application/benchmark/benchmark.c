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
#include "bk_micro_log.h"
#include "bk_eots_hd.h"
#include "bk_fbkflow.h"
#include "autofocus.h"
#include "bk_mblock.h"
#include "awoke_kalman.h"
#include "img_sensor.h"
#include "bk_nvm.h"
#include "bk_epc.h"
#include "md5.h"
#include "bk_quickemb.h"
#include "bk_strucp.h"
#include "ring_queue.h"
#include "memtrace.h"



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
		return et_finish;
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
		return et_ok;
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

err_type benchmark_event_chn_work(void *data)
{
	int rc;
	awoke_worker_context *ctx = data;
	awoke_worker *wk = ctx->worker;
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
		return err_fail;
	}

	ret = awoke_event_pipech_create(t->evl, &t->pipe_channel);
	if (ret != et_ok) {
		log_err("event channel create error, ret %d", ret);
		return err_fail;
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

static err_type benchmark_test_work(void *data)
{
	awoke_worker_context *ctx = data;
	awoke_worker *wk = ctx->worker;
	log_debug("%s run", wk->name);
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

err_type worker_lock_test(void       *data)
{
	err_type ret;
	awoke_worker_context *ctx = data;
	awoke_worker *wk = ctx->worker;
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
		return err_oom;
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

	awoke_queue_free(&q);

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

static void bk_string_pack_test(uint8_t *buffer, char *string, int length)
{
	pkg_push_stris(string,buffer,length);
}


static err_type benchmark_sscanf_test(int argc, char *argv[])
{
    /*
	int length;
	char *str = "Content-Length: 14615\r\ndsadsadsadasdsadasd";

	sscanf(str, "Content-Length: %d%*s", &length);
	log_debug("length:%d", length);
    */

    /*
    int raw = 66534;
    uint16_t volt;
    uint16_t data;

    volt = raw;
    log_debug("volt:%d", volt);
    data = htons(volt/10);
    log_debug("data:%d", data);

	uint8_t mark1, mark2;
	awoke_buffchunk *buffer;
	uint8_t bufferout[32];
	char string1[32] = "K172";
	uint8_t *pos;

	log_debug("strlen:%d", strlen(string1));

	buffer = awoke_buffchunk_create(32);
	pos = (uint8_t *)buffer->p;

	memset(buffer->p, 0x0, buffer->size);
	mark1 = 0x24;
	mark2 = 0x25;
	awoke_hexdump_trace(buffer->p, buffer->size);
	awoke_hexdump_trace(string1, strlen(string1));
	pkg_push_byte(mark1, pos);
	pkg_push_byte(mark2, pos);
	pkg_push_stris(string1, pos, strlen(string1));
	awoke_hexdump_trace(buffer->p, buffer->size);
	
	pos = (uint8_t *)buffer->p;
	pkg_pull_stris(bufferout, pos, strlen(string1));
	awoke_hexdump_trace(bufferout, 32);
	log_debug("string:%s", bufferout);

	memset(buffer, 0x0, 32);
	bk_string_pack_test((uint8_t *)buffer, string1, strlen(string1));
	awoke_hexdump_trace(buffer, 32);
	
	awoke_buffchunk_free(&buffer);
	pkg_push_stris(string1, pos, strlen(string1));
	log_debug("buffer:%s", buffer);
	awoke_hexdump_trace(buffer, 32);
	pos = buffer;
	pkg_pull_stris(bufferout, pos, strlen(string1));
	log_debug("bufferout:%s", bufferout);
	*/

    uint16_t y, h;
    uint16_t _y, _h, y_pad, h_pad;

    y = atoi(argv[2]);
    h = atoi(argv[3]);

    y_pad = (y+4)%8;
    _y = (y+4) & ~(0x7);
    if ((y_pad+h)%4) {
        h_pad = 4 - (y_pad+h)%4;
    } else {
        h_pad = 0;
    }
    _h = y_pad + h_pad + h;
    bk_debug("ROI y:%d h:%d -> CCDRange(%d-%d)", y, h, y+4, y+4+h);
    bk_debug("CCD y:%d h:%d -> CCDRange(%d-%d)", _y, _h, _y, _y+_h);
    bk_debug("pad:%d %d", y_pad, h_pad);
    
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
		return err_oom;
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

static err_type benchmark_queue_test(int argc, char *argv[])
{
    int a, b, c, d, e, f, g, h, i, j, *p, u;
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

	return et_ok;
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

void fifo_test_value_handle(void *data, int width, char *buff, int len)
{
    int i;
    int d;
	awoke_fifo *f = data;
    
    build_ptr bp = build_ptr_init(buff, len);

    build_ptr_string(bp, "value:");
    for (i=0; i<awoke_fifo_size(f); i++) {
        awoke_fifo_get(f, &d, i);
        build_ptr_format(bp, "%*d", width, d);
    }
}

static err_type benchmark_fifo_test(int argc, char *argv[])
{
	int x, a, b, c, d, e, f, g, h, i, j;

	awoke_fifo *fifo = awoke_fifo_create(sizeof(int), 8, 0x0);
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

	bk_debug("enq %d", a);
	awoke_fifo_enqueue(fifo, &a);
	bk_debug("enq %d", b);
	awoke_fifo_enqueue(fifo, &b);
	bk_debug("enq %d", c);
	awoke_fifo_enqueue(fifo, &c);
	bk_debug("enq %d", d);
	awoke_fifo_enqueue(fifo, &d);
	bk_debug("enq %d", e);
	awoke_fifo_enqueue(fifo, &e);
	bk_debug("enq %d", f);
	awoke_fifo_enqueue(fifo, &f);
	bk_debug("enq %d", g);
	awoke_fifo_enqueue(fifo, &g);
	bk_debug("enq %d", h);
	awoke_fifo_enqueue(fifo, &h);
	bk_debug("enq %d", i);
	awoke_fifo_enqueue(fifo, &i);
	bk_debug("enq %d", j);
	awoke_fifo_enqueue(fifo, &j);

	awoke_fifo_dump(fifo);

	awoke_fifo_dequeue(fifo, &x);
	bk_debug("deq %d", x);
	awoke_fifo_dequeue(fifo, &x);
	bk_debug("deq %d", x);
	awoke_fifo_dequeue(fifo, &x);
	bk_debug("deq %d", x);
	
	awoke_fifo_dump(fifo);
	
	awoke_fifo_dequeue(fifo, &x);
	bk_debug("deq %d", x);
	awoke_fifo_dequeue(fifo, &x);
	bk_debug("deq %d", x);
	awoke_fifo_dequeue(fifo, &x);
	bk_debug("deq %d", x);

	awoke_fifo_dump(fifo);
	
	awoke_fifo_dequeue(fifo, &x);
	bk_trace("deq %d", x);
	awoke_fifo_dequeue(fifo, &x);
	bk_trace("deq %d", x);

	bk_trace("enq %d", e);
	awoke_fifo_enqueue(fifo, &e);
	bk_trace("enq %d", f);
	awoke_fifo_enqueue(fifo, &f);
	bk_trace("enq %d", h);
	awoke_fifo_enqueue(fifo, &h);

	awoke_fifo_dump(fifo);

	return et_ok;
}

typedef struct _bk_btree_node {
	int v;
	awoke_btree_node node;
} bk_btree_node;

static int benchmark_btree_compare(const void *d1, const void *d2)
{
	bk_btree_node *n1 = (bk_btree_node *)d1;
	bk_btree_node *n2 = (bk_btree_node *)d2;

	return (n1->v - n2->v);
}

static void benchmark_btree_dump(void *data)
{
	bk_btree_node *n = (bk_btree_node *)data;
	bk_trace("v:%d", n->v);
}

static err_type benchmark_btree_test(int argc, char *argv[])
{
	awoke_btree tree;
	bk_btree_node n1, n2, n3, n4;

	n1.v = 1;
	n1.node.data = &n1;
	n1.node.left = NULL;
	n1.node.right = NULL;

	n2.v = 2;
	n2.node.data = &n2;
	n2.node.left = NULL;
	n2.node.right = NULL;

	n3.v = 3;
	n3.node.data = &n3;
	n3.node.left = NULL;
	n3.node.right = NULL;

	n4.v = 4;
	n4.node.data = &n4;
	n4.node.left = NULL;
	n4.node.right = NULL;

	awoke_btree_init(&tree, benchmark_btree_compare);

	awoke_btree_add(&tree, &n4.node);
	awoke_btree_add(&tree, &n2.node);
	awoke_btree_add(&tree, &n1.node);
	awoke_btree_add(&tree, &n3.node);
	awoke_btree_add(&tree, &n2.node);

	awoke_btree_traverse(&tree, benchmark_btree_dump);

	return et_ok;
}

static err_type benchmark_md5_test(int argc, char *argv[])
{
	char *p;
	char buf[1024];
	char md5in[32];
	char md5out[16];
	char imeibcd[16];
	char *imeistr = "0123456789012347";
	char *slatstr = "secret";
	uint8_t version = 0;
	uint8_t slat_byte[8] = {0x0};
	uint8_t imei_byte[8] = {0x0};
	uint8_t *pos = (uint8_t *)buf;
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
	md5((const uint8_t *)md5in, 16, (uint8_t *)md5out);

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
	struct timeval tv;
	int fd_server, fd_client;
	struct sockaddr_in servaddr;
	
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

    memset(&tv, 0x0, sizeof(tv));
    tv.tv_sec = 1;
    tv.tv_usec = 0;

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
	log_debug(
"fd[0]:%d %d %d", fds[0].fd, fds[0].events, fds[0].revents);
	log_debug(
"fd[1]:%d %d %d", fds[1].fd, fds[1].events, fds[1].revents);

	fds[0].events |= POLLOUT;

	ret = poll(fds, 2, 1000);
	log_debug(
"fd[0]:%d %d %d", fds[0].fd, fds[0].events, fds[0].revents);
	log_debug(
"fd[1]:%d %d %d", fds[1].fd, fds[1].events, fds[1].revents);
	*/

	fd_set readfds, writefds, errfds;
	int max_fd = 0;
	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	FD_ZERO(&errfds);

	FD_SET(fd_server, &readfds);
	FD_SET(fd_client, &readfds);

	select(max_fd + 1, &readfds, &writefds, &errfds, &tv);
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
	select(max_fd + 1, &readfds, &writefds, &errfds, &tv);
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

	return et_ok;
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
	listtest a, b, d, e, f, g, h;
	awoke_list xlist;

	list_init(&xlist);

	a.d = 1;
	b.d = 2;
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
			sprintf(c->name, "%s", rn);
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
		sprintf(c->name, "%s", rn);
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

static err_type logfile_cache_write_message(void *ctx)
{
	log_trace("write message");

	if (logfile_full(&logfilem)) {
		logfile_curr2cache(&logfilem);
	}

	logfilem.currsize += 553;
	log_trace("currsize:%d", logfilem.currsize);

	logfile_cache_dump(&logfilem);

	return et_ok;
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
	return et_ok;
}

static err_type log_cache_work(void *ctx)
{
	bk_debug("cache test");
	return et_ok;
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

	return et_ok;
}

static err_type benchmark_micro_log_test(int argc, char *argv[])
{
	bk_microlog_info("benchmark");
	bk_microlog_debug("micro log test");
	bk_microlog_err("we heve some error");
	bk_microlog_info("dd");
	bk_microlog_info("123456");

	bk_microlog_info("123");
	bk_microlog_info("12345");
	bk_microlog_err("1234");
	bk_microlog_err("123456");
	bk_microlog_err("123456");
	bk_microlog_err("1234");
	bk_microlog_err("12345");
	bk_microlog_debug("12345678");

	bk_microlog_info("bb");

	bk_microlog_info("bbb");

	bk_microlog_info("bbbb");

	bk_microlog_info("bbbbb");

	bk_microlog_info("bbbbbb");

	bk_microlog_info("bbbbbbb");

	bk_microlog_print();
	
	return et_ok;
}

static err_type benchmark_eots_hd_test(int argc, char *argv[])
{
	int opt, len = 0;
	char *input_string;
	char *input_filepath;
	uint8_t *pos;
	uint8_t buffer[256] = {0x0};
	uint8_t cmd1[6] = {0x81, 0x01, 0x37, 0x02, 0x01, 0xff};
	//uint8_t cmd2[6] = {0x81, 0x01, 0x37, 0x02, 0x02, 0xff};
	uint8_t cmd3[6] = {0x81, 0x01, 0x46, 0x00, 0x01, 0xff};
	//uint8_t cmd4[6] = {0x81, 0x01, 0x46, 0x00, 0x04, 0xff};
	//uint8_t cmd5[6] = {0x81, 0x01, 0x63, 0x00, 0x04, 0xff};
	//uint8_t cmd6[6] = {0x81, 0x01, 0x66, 0x00, 0x04, 0xff};
	//uint8_t cmd7[6] = {0x81, 0x01, 0x66, 0x00, 0x02, 0xff};
	uint8_t cmd8[6] = {0x81, 0x01, 0x39, 0x00, 0x03, 0xff};
	//uint8_t cmd9[6] = {0x81, 0x01, 0x39, 0x02, 0x00, 0xff};

		
	static const struct option long_opts[] = {
		{"input",		required_argument,	NULL,	arg_eots_ecmd_input},
		{"fix",			required_argument,	NULL,	arg_eots_ecmd_fix},
		{"file",		required_argument,	NULL,	arg_eots_ecmd_file},
        {NULL, 0, NULL, 0}
    };

	while ((opt = getopt_long(argc, argv, "?h", long_opts, NULL)) != -1)
    {
        switch (opt)
        {
        	case arg_eots_ecmd_input:
				input_string = optarg;
				bk_trace("input string:%s strlen:%d", input_string, strlen(input_string));
				len = awoke_string_to_hex(input_string, buffer, strlen(input_string));
				bk_trace("hexlen:%d", len);
				break;

			case arg_eots_ecmd_fix:
				pos = buffer;
				pkg_push_bytes(cmd1, pos, 6);
				pkg_push_bytes(cmd3, pos, 6);
				pkg_push_bytes(cmd8, pos, 6);
				len = pos - buffer;
				break;

			case arg_eots_ecmd_file:
				input_filepath = optarg;
				bk_trace("input filepath:%s", input_filepath);
				//len = bk_eots_hd_file2buffer(input_filepath, buffer);
				break;
				
			case '?':
			case 'h':
			default:
				break;
        }
    }

	awoke_hexdump_trace(buffer, len);
	//bk_eots_hd_ecmd_process(buffer, len);
	bk_eots_hd_syscon(buffer, len);
	return et_ok;
}

static void afifo_test_value_handle(void *data, int width, char *buff, int len)
{
    int i;
    char d;
	awoke_fifo *f = data;
    
    build_ptr bp = build_ptr_init(buff, len);

    build_ptr_string(bp, "value:");
    for (i=0; i<awoke_fifo_size(f); i++) {
        awoke_fifo_get(f, &d, i);
        build_ptr_format(bp, "%*c", width, d);
    }
}

static err_type benchmark_array_fifo_test(int argc, char *argv[])
{
	awoke_fifo fifo;
	char data[3] = {0x0};
	char a, b, c, x1, x2, x3;

	a = 'A';
	b = 'B';
	c = 'C';

	awoke_fifo_init(&fifo, data, sizeof(char), 3, AWOKE_FIFO_F_RBK);
	awoke_fifo_dumpinfo_set(&fifo, 4, afifo_test_value_handle, NULL);

	awoke_afifo_enqueue(&fifo, &c);
	awoke_afifo_enqueue(&fifo, &a);
	awoke_afifo_enqueue(&fifo, &b);

	awoke_afifo_enqueue(&fifo, &a);
	awoke_afifo_enqueue(&fifo, &c);
	awoke_afifo_enqueue(&fifo, &a);
	awoke_afifo_enqueue(&fifo, &b);

	awoke_fifo_get(&fifo, &x1, 0);
	awoke_fifo_get(&fifo, &x2, 1);
	awoke_fifo_get(&fifo, &x3, 2);

	bk_debug("x1:%c x2:%c x3:%c", x1, x2, x3);

	if ((x1=='C') && (x2=='A') && (x3=='B')) {
		bk_info("find BAC");
	}

	awoke_fifo_dump(&fifo);

	return et_ok;
}

static err_type benchmark_valuemem_test(int argc, char *argv[])
{

typedef struct {
	uint32_t a1;
	uint32_t a2;
} valuemem_A;

typedef struct {
	uint16_t a1_hi;
	uint16_t a1_lo;
	uint16_t a2_hi;
	uint16_t a2_lo;
} valuemem_B;

	valuemem_A a, atest;
	valuemem_B b;

	b.a1_hi = 10;
	b.a1_lo = 12;
	b.a2_hi = 6;
	b.a2_lo = 7;

	a.a1 = *(uint32_t *)&b.a1_hi;
	memcpy(&atest, &b.a1_hi, 4);

	bk_debug("a.a1:0x%x %d", a.a1, a.a1);
	bk_debug("a.a1:0x%x %d", atest.a1, atest.a1);

	return et_ok;
}

static int fibonacci(int n)
{
	if ((n==1) || (n==2)) {
		return 1;
	}

	return fibonacci(n-1) + fibonacci(n-2);
}

static err_type benchmark_fibonacci_test(int argc, char *argv[])
{
	int x = fibonacci(8);

	bk_debug("fibonacci 8: %d", x);

	return et_ok;
}

static err_type benchmark_kalman_test(int argc, char *argv[])
{
	int i;
	struct awoke_kalman1d kmf;
	double xhat;
	uint32_t ms[30] = {
		0x03ccd00c, 0x03f94ebe, 0x03d22f26, 0x03f7e8ae, 0x03e4a09e, 0x03eb1f94, 
		0x03e5785a, 0x03dfea28, 0x03dab934, 0x03f2f9f8, 0x03c0d2de, 0x03df919c,
		0x03c88cc8, 0x03e28786, 0x03bafbf4, 0x03c4905c, 0x03db5ad2, 0x03a934b8,
		0x03ca7664, 0x03d8d71e, 0x03c1d9e8, 0x03c7e3d6, 0x03dc9f9c, 0x03b6a898,
		0x03cb065e, 0x03c9b07e, 0x03af938a, 0x03c57d1c, 0x03c958e2, 0x0420e710};
	
	awoke_kalman1d_init(&kmf, 0x02000000, 10000, 0, 100);

	for (i=0; i<30; i++) {
		xhat = awoke_kalman1d_filter(&kmf, ms[i]);
		bk_trace("estimate:%f", xhat);
	}

	return et_ok;
}

double hqmath_abs(double j)
{
    if (j < 0) {
       j = j *(-1);
    }
	
    return (j);
}

static void mdiff_calculate(awoke_queue *q)
{
	int i;
	uint32_t v1, v2, sum, absdiff, mdiff;
	int32_t diff;
	int size = awoke_queue_size(q);

	//bk_trace("size:%d", size);

	if (size < 2)
		return;

	sum = 0;
	
	for (i=1; i<size; i++) {
		awoke_queue_get(q, i-1, &v1);
		awoke_queue_get(q, i, &v2);
		diff = v2-v1;
		absdiff = hqmath_abs(diff);
		sum += absdiff;
		//bk_trace("v1:%d v2:%d diff:%d absdiff:%d sum:%d", v1, v2, diff, absdiff, sum);
	}

	mdiff = sum/size;

	bk_trace("sum:%d, \tmdiff:%d", sum, mdiff);
}

/*
static void mae_calculate(awoke_queue *q)
{
	int i;
	uint32_t v, avg;
	uint32_t sum = 0;
	int size = awoke_queue_size(q);

	for (i=0; i<size; i++) {
		awoke_queue_get(q, i, &v);
		sum += v;
	}

	avg = sum/size;

	for (i=0; i<size; i++) {
		awoke_queue_get(q, i, &v);
		sum += v;
	}
}*/

static err_type benchmark_mae_test(int argc, char *argv[])
{
	int i;
	struct awoke_kalman1d kmf;
	awoke_queue *q = awoke_queue_create(sizeof(uint32_t), 30, AWOKE_QUEUE_F_RB);

	awoke_kalman1d_init(&kmf, 0x02000000, 10000, 0, 100);

	uint32_t ms[30] = {
		0x03ccd00c, 0x03f94ebe, 0x03d22f26, 0x03f7e8ae, 0x03e4a09e, 0x03eb1f94, 
		0x03e5785a, 0x03dfea28, 0x03dab934, 0x03f2f9f8, 0x03c0d2de, 0x03df919c,
		0x03c88cc8, 0x03e28786, 0x03bafbf4, 0x03c4905c, 0x03db5ad2, 0x03a934b8,
		0x03ca7664, 0x03d8d71e, 0x03c1d9e8, 0x03c7e3d6, 0x03dc9f9c, 0x03b6a898,
		0x03cb065e, 0x03c9b07e, 0x03af938a, 0x03c57d1c, 0x02d958e2, 0x02b0e710};

	for (i=0; i<30; i++) {
		awoke_queue_enq(q, &ms[i]);
		mdiff_calculate(q);
	}

	return et_ok;
}

static err_type benchmark_checksum_test(int argc, char *argv[])
{
	int num, size;
	uint16_t checksum;
	uint32_t value, *p;
	
	uint32_t data[] = {
		0x00000000,
		0x00000000,
		0x5a5a0123,
		0x24814520,
		0x01003e05,
		0x06a40400,
		0x09607341,
		0x00001770,
		0x5a5a5678,
	};

	uint8_t data2[] = {
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00,
		0x5a, 0x5a, 0x23, 0x01,
		0x81, 0x24, 0x20, 0x45,
		0x00, 0x01, 0x05, 0x3e,
		0xa4, 0x06, 0x00, 0x04,
		0x60, 0x09, 0x41, 0x73, 
		0x00, 0x00, 0x70, 0x17, 
		0x5a, 0x5a, 0x78, 0x56,
	};

	p = (uint32_t *)data2;
	value = *(p+8);
	value = htonl(value);
	bk_trace("value 0x%x", value);

	num = array_size(data);
	size = sizeof(uint32_t)*num;

	checksum = awoke_checksum_u16((uint8_t *)&data, size);
	bk_debug("num:%d size:%d checksum:0x%x", num, size, checksum);

	checksum = awoke_checksum_u16((uint8_t *)&data2, size);
	bk_debug("num:%d size:%d checksum:0x%x", num, size, checksum);

	return et_ok;
}

static err_type benchmark_sign_exten(int argc, char *argv[])
{
	char string[16] = {'\0'};
	uint8_t data[2] = {0xFC, 0X0B};
	uint32_t edata;
	int32_t sdata;
	uint8_t value1, value2, value3, value4;
	int symbol = 0;
	double tmp;

	edata = (data[0]<<4)|data[1];
	if (edata & 0x800) {
		bk_debug("negative");
		sdata = (edata & ~(0x800))*(-1);
	} else {
		sdata = edata;
	}

	bk_debug("edata:0x%x %d", edata, edata);
	bk_debug("sdata:0x%x %d", sdata, sdata);

	bk_debug("data[0]:0x%x data[1]:0x%x", data[0], data[1]);

	if (mask_exst(edata, 0x800)) {
		symbol = 1;
	}

	value1 = (edata & 0x7F8) >> 3;
	value2 = (edata & 0x4) >> 2;
	value3 = (edata & 0x2) >> 1;
	value4 = edata & 0x1;

	bk_debug("value1:0x%x value2:0x%x value3:0x%x value4:0x%x", value1, value2, value3, value4);

	tmp = symbol*(-256) + value1 + 0.5*value2 + 0.25*value3 + 0.125*value4;
	bk_debug("tmp:%f", tmp);

	awoke_sprintf(string, "%f", tmp);
	bk_debug("string:%s", string);
		
	return et_ok;
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
		return err_open;
	}

	rc = read(fd, buffer, 1024);
	bk_debug("read:%d", rc);
	close(fd);

	checksum = config_checksum((uint8_t *)buffer, rc);
	bk_debug("checksum:0x%x", checksum);

	cfg = (struct CameraConfig *)buffer;
	awoke_hexdump_trace(buffer, rc);
	bk_debug("mark:0x%x", cfg->mark);
	bk_debug("goal:%d min:%d max:%d", cfg->goal, cfg->goal_min, cfg->goal_max);
	return et_ok;
}

static err_type benchmark_sensorconfig_test(int argc, char *argv[])
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
		return err_open;
	}

	rc = read(fd, buffer, 1024);
	bk_debug("read:%d", rc);
	close(fd);
	checksum = config_checksum((uint8_t *)buffer, rc);
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

static err_type benchmark_memmove_test(int argc, char *argv[])
{
	uint8_t buffer[16] = {
		0x01, 0x02, 0x03, 0x04, 0x00, 0x00, 0x00, 0x08,
		0x08, 0x07, 0x06, 0x05, 0x00, 0x00, 0x00, 0x00,
	};

	awoke_hexdump_trace(buffer, 16);
	memmove(buffer, buffer+7, 16);
	awoke_hexdump_trace(buffer, 16);
	return et_ok;
}

#define _init __attribute__((unused, section(".myinit.1")))
#define DECLARE_INIT(func) init_fn_t _fn_##func _init = func

#define RT_SECTION(x)	 __attribute__((section(x)))
typedef int (*init_fn_t)(void);
#define INIT_EXPORT(fn, level) \
	__attribute__((used)) const init_fn_t __rt_init_##fn RT_SECTION(".rti_fn."level) = fn
#define INIT_BOARD_EXPORT(fn)           INIT_EXPORT(fn, "1")

init_fn_t _init_start;
init_fn_t _init_end;


static int rti_start(void)
{
	return 0;
}
INIT_EXPORT(rti_start, "0");

static int rti_end(void)
{
	return 0;
}
INIT_EXPORT(rti_end,"7");

static int rti_board_start(void)
{
    return 0;
}
INIT_EXPORT(rti_board_start, "0.end");

static int rti_board_end(void)
{
    return 0;
}
INIT_EXPORT(rti_board_end, "1.end");

static int bk_hw_timer_init(void)
{
	bk_debug("timer init");
	return 0;
}	
INIT_BOARD_EXPORT(bk_hw_timer_init);

static int bk_hw_spi_init(void)
{
	bk_debug("spi init");
	return 0;
}	
INIT_BOARD_EXPORT(bk_hw_spi_init);


static err_type benchmark_attribute_test(int argc, char *argv[])
{
	volatile const init_fn_t *fn_ptr;

	//bk_debug("start:0x%x", __rt_init_rti_board_start);
	//bk_debug("end:0x%x", __rt_init_rti_board_end);

    for (fn_ptr = &__rt_init_rti_board_start; fn_ptr < &__rt_init_rti_board_end; fn_ptr++)
    {
        (*fn_ptr)();
    }

	//fn_ptr = &__rt_init_rti_board_start

	return et_ok;
}

struct ptrfunc_test {
	int* (*func)(int *x);
};

int *func(int *x)
{
	return x;
}

unsigned int BKDRHash(const char *str)
{
	unsigned int seed = 131;
	unsigned int hash = 0;

	while (*str)
		hash = hash * seed + (*str++);

	return hash & 0x7FFFFFFF;
}

err_type benchmark_bkdrhash_test(int argc, char *argv[])
{
	int i;
	
	struct bkdrhash_test {
		char *str;
		unsigned int key;
	};

	struct bkdrhash_test hashs[8] = {
		{"TaskACount", 0},
		{"TaskBCount", 0},
		{"AEControl", 0},
		{"Temperate", 0},
		{"XferReadCount", 0},
		{"XferWriteCount", 0},
		{"IICReadCount", 0},
		{"IICWriteCount", 0},
		{"IICWriteCount", 0},
	};

	for (i=0; i<8; i++) {
		hashs[i].key = BKDRHash(hashs[i].str)%128;
		bk_debug("%s %d", hashs[i].str, hashs[i].key);
	}
	
	return et_ok;
}

err_type benchmark_ringbuffer(int argc, char *argv[])
{
	int fd;
	char buffer[512];
	struct awoke_ringbuffer rb;

	char *strings[] = {
		"A12345\r\n",
		"B123456789\r\n",
		"C123456789123456789\r\n",
		"D1234567891234567891234567891234567891234567891234567891234567891234\r\n",
		"E12345678912345678912345678912345678912345\r\n",
		"F123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123456789123\r\n",
	};

	memset(buffer, 0x0, 512);
	awoke_ringbuffer_init(&rb, buffer, 512);

	for (int j=0; j<200; j++) {
		for (int i=0; i<array_size(strings); i++) {
			awoke_ringbuffer_write(&rb, strings[i], strlen(strings[i]));
		}
	}

	awoke_hexdump_debug(buffer, 512);

	fd = open("RingBuffer", O_WRONLY|O_CREAT, 0777);
	write(fd, buffer, 512);
	close(fd);

	return et_ok;
}

int main(int argc, char *argv[])
{
	int opt;
	int loglevel = LOG_TRACE;
	bencmark_func bmfn = NULL;

	static const struct option long_opts[] = {
		{"loglevel",			required_argument,	NULL,	'l'},
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
		{"btree-test",			no_argument,		NULL,	arg_btree_test},
		{"md5-test",			no_argument, 		NULL,	arg_md5_test},
		{"socket-poll-test",	no_argument,		NULL,	arg_socket_poll_test},
		{"list-test",			no_argument,		NULL,	arg_list_test},
		{"logfile-cache-test",	no_argument, 		NULL,	arg_logfile_cache_test},
		{"filecache-test",		no_argument, 		NULL,	arg_filecache_test},
		{"log-color-test",		no_argument,		NULL,	arg_log_color_test},
		{"log-cache-test",		no_argument,		NULL,	arg_log_cache_test},
		{"micro-log-test",		no_argument,		NULL,	arg_macro_log_test},
		{"eots-hd-test",		no_argument,		NULL,	arg_eots_hd_test},
		{"array-fifo-test",		no_argument,		NULL,	arg_array_fifo_test},
		{"fbkflow-test",		no_argument,		NULL,	arg_fbkflow_test},
		{"autofocus-test",		no_argument,		NULL,	arg_autofocus_test},
		{"valuemem-test",		no_argument,		NULL,	arg_valuemem_test},
		{"fibonacci-test",		no_argument,		NULL,	arg_fibonacci_test},
		{"version-test",		no_argument,		NULL,	arg_version_test},
		{"mblock-test",			no_argument,		NULL,	arg_mblock_test},
		{"kalman-test",			no_argument,		NULL,	arg_kalman_test},
		{"mae-test",			no_argument,		NULL,	arg_mae_test},
		{"checksum-test",		no_argument,		NULL,	arg_checksum_test},
		{"imgsensor-test",		no_argument,		NULL,	arg_imgsensor_test},
		{"nvm-test",			no_argument,		NULL,	arg_nvm_test},
		{"epc-test",			no_argument,		NULL,	arg_epc_test},
		{"sign-exten",			no_argument,		NULL,	arg_sign_exten},
		{"cameraconfig-test",   no_argument,        NULL,   arg_cameraconfig_test},
		{"sensorconfig-test",   no_argument,        NULL,   arg_sensorconfig_test},
		{"memmove-test",   		no_argument,        NULL,   arg_memmove_test},
		{"attribute-test",		no_argument,		NULL,	arg_attribute_test},
		{"quickemb-test",		no_argument,		NULL,   arg_quickemb_test},
		{"strucp-test",         no_argument,        NULL,   arg_strucp_test},
		{"bkdrhash-test",       no_argument,        NULL,   arg_bkdrhash_test},
		{"ringq-test",			no_argument, 		NULL,	arg_ringq_test},
		{"ringbuffer",			no_argument,		NULL,	arg_ringbuffer},
		{"memtrace",			no_argument,		NULL,	arg_memtrace_test},
		{NULL, 0, NULL, 0}
    };	

	while ((opt = getopt_long(argc, argv, "l:m:?h-", long_opts, NULL)) != -1)
    {
        switch (opt)
        {
        	case 'l':
				loglevel = atoi(optarg);
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

			case arg_btree_test:
				bmfn = benchmark_btree_test;
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

			case arg_macro_log_test:
				bmfn = benchmark_micro_log_test;
				break;

			case arg_eots_hd_test:
				bmfn = benchmark_eots_hd_test;
				goto run;

			case arg_array_fifo_test:
				bmfn = benchmark_array_fifo_test;
				break;

			case arg_fbkflow_test:
				bmfn = benchmark_fbkflow_test;
				break;

			case arg_autofocus_test:
				bmfn = benchmark_autofocus_test;
				break;

			case arg_valuemem_test:
				bmfn = benchmark_valuemem_test;
				break;

			case arg_fibonacci_test:
				bmfn = benchmark_fibonacci_test;
				break;

			case arg_mblock_test:
				bmfn = benchmark_mblock_test;
				//bmfn = benchmark_version_test;
				bk_trace("mblock test");
				break;

			case arg_kalman_test:
				bmfn = benchmark_kalman_test;
				break;

			case arg_mae_test:
				bmfn = benchmark_mae_test;
				break;

			case arg_checksum_test:
				bmfn = benchmark_checksum_test;
				break;

			case arg_imgsensor_test:
				bmfn = benchmark_imgsensor_test;
				break;

			case arg_nvm_test:
				bmfn = benchmark_nvm_test;
				break;

			case arg_epc_test:
				bmfn = benchmark_epc_test;
				break;

			case arg_sign_exten:
				bmfn = benchmark_sign_exten;
				break;
				
			case arg_cameraconfig_test:
				bmfn = benchmark_cameraconfig_test;
				break;

			case arg_sensorconfig_test:
				bmfn = benchmark_sensorconfig_test;
				break;
				
			case arg_memmove_test:
				bmfn = benchmark_memmove_test;
				break;

			case arg_attribute_test:
				bmfn = benchmark_attribute_test;
				break;

			case arg_quickemb_test:
				bmfn = benchmark_quickemb_test;
				break;

            case arg_strucp_test:
                bmfn = benchmark_strucp_test;
                break;

			case arg_bkdrhash_test:
				bmfn = benchmark_bkdrhash_test;
				break;

			case arg_ringq_test:
				bmfn = benchmark_ringq_test;
				break;

			case arg_ringbuffer:
				bmfn = benchmark_ringbuffer;
				break;

			case arg_memtrace_test:
				bmfn = benchmark_memtrace_test;
				goto run;

            case '?':
            case 'h':
            default:
               	usage(AWOKE_EXIT_SUCCESS);
				break;
        }
    }

run:
	awoke_log_init(loglevel, LOG_M_ALL, LOG_D_STDOUT);
	bmfn(argc, argv);

	return 0;
}
