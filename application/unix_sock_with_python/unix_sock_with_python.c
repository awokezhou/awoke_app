#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/time.h>
#include <netinet/tcp.h>
#include <sys/wait.h>  
#include <semaphore.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <sys/mman.h>
#include <sys/sem.h>
#include <sys/un.h>


#include "awoke_rtmsg.h"
#include "awoke_event.h"
#include "awoke_list.h"
#include "awoke_type.h"
#include "awoke_error.h"
#include "awoke_log.h"
#include "awoke_socket.h"
#include "awoke_macros.h"
#include "awoke_worker.h"

typedef struct _listener {
	char *name;
	int fd;
	awoke_event event;
}listener;

typedef struct _pthread_params {
	pthread_t id;
	pthread_mutex_t mut;
	pthread_cond_t cond;
	bool f_running;
}pthread_params;

pthread_params pthread1, pthread2;
awoke_worker *worker1, *worker2;


typedef struct _connector {
	awoke_event event;
	listener *lsr;
}connector;

#define SOCK_ADDR "unix_sock_with_python"

void thread_resume(pthread_params *param)
{
    if (!param->f_running)
    {  
        pthread_mutex_lock(&param->mut);
        param->f_running = TRUE;
        pthread_cond_signal(&param->cond);
		log_debug("thread_resume");
        pthread_mutex_unlock(&param->mut);
    } else {
		log_err("status already TRUE");
	}
}

void thread_pause(pthread_params *param)
{
    if (param->f_running)
    {  
        pthread_mutex_lock(&param->mut);
        param->f_running = FALSE;
		pthread_cond_signal(&param->cond);
		log_debug("thread_pause");
        pthread_mutex_unlock(&param->mut);
    } else {
		log_err("status already FALSE");
	}
}

void pthread_wait_running(pthread_params *param)
{
	pthread_mutex_lock(&param->mut);
	while (!param->f_running)
	{
		log_debug("pthread_cond_wait");
		pthread_cond_wait(&param->cond, &param->mut);
	}
	pthread_mutex_unlock(&param->mut);	
}

void hypnos_worker_start()
{
	awoke_worker_start(worker1);
	awoke_worker_start(worker2);
}

void hypnos_worker_suspend()
{
	awoke_worker_suspend(worker1);
	awoke_worker_suspend(worker2);
}

void hypnos_worker_resume()
{
	awoke_worker_resume(worker1);
	awoke_worker_resume(worker2);	
}

void hypnos_worker_stop()
{
	awoke_worker_stop(worker1);
	awoke_worker_stop(worker2);		
}

err_type server_start(listener *lsr)
{
	awoke_event *event;
	awoke_event_loop *evl;
	int client_fd;
	int rc;
	char buff[1024];
	
	evl = awoke_event_loop_create(EVENT_QUEUE_SIZE);
    if (!evl)
    {
        log_err("event loop create error");
        return et_evl_create;
    }
	awoke_event_add(evl, lsr->fd, EVENT_LISTENER, EVENT_READ, lsr);

	while (1) {
		awoke_event_wait(evl, 10);
		awoke_event_foreach(event, evl) {
		
			if (!awoke_event_ready(event, evl))
                continue;

			memset(buff, 0x0, 1024);

			if (event->type == EVENT_LISTENER) {
				log_debug("EVENT_LISTENER");
				client_fd = awoke_socket_accept_unix(event->fd);
				connector cnr;
				awoke_event *new_event;
				new_event = &cnr.event;
				new_event->fd = client_fd;
			    new_event->type = EVENT_CONNECTION;
			    new_event->mask = EVENT_EMPTY;
			    new_event->status = EVENT_NONE;    
				awoke_event_add(evl, client_fd, EVENT_CONNECTION, EVENT_READ, &cnr);
			} else if (event->type == EVENT_CONNECTION) {
				log_debug("EVENT_CONNECTION");
				rc = read(event->fd, buff, 1024);
				log_info("buff %s", buff);
				if (!strcmp(buff, "worker start")) {
					hypnos_worker_start();
				} else if (!strcmp(buff, "worker suspend")) {
					hypnos_worker_suspend();
				} else if (!strcmp(buff, "worker resume")) {
					hypnos_worker_resume();
				} else if (!strcmp(buff, "worker stop")) {
					hypnos_worker_stop();
				}
			} else if (event->type == EVENT_NOTIFICATION) {
				/**/
			} else if (event->type == EVENT_CUSTOM) {
				/**/
			}
		}
	}
}

err_type read_obd_thread()
{
	log_debug("read obd");
	return et_ok;
}

err_type *read_eng_thread(void *context)
{
	awoke_worker_context *ctx = context;
	awoke_worker *wk = ctx->worker;

	while (!awoke_worker_should_stop(wk)) {
		awoke_worker_should_suspend(wk);
		log_debug("read eng");
		sleep(5);
	}	
}

void server_run()
{
	int fd;
	listener lsr;
	awoke_event *event;
	
	log_level(LOG_DBG);

	log_debug("main");

#if 0	
	pthread_mutex_init(&pthread1.mut, NULL);
	pthread_cond_init(&pthread1.cond, NULL);
	pthread1.f_running = TRUE;
	if(pthread_create(&pthread1.id, NULL, read_obd_thread, &pthread1))  
    {  
        log_err("Can't create OBD thread!");  
        return 1;  
    } 
	
	pthread_mutex_init(&pthread2.mut, NULL);
	pthread_cond_init(&pthread2.cond, NULL);
	pthread2.f_running = TRUE;
	if(pthread_create(&pthread2.id, NULL, read_eng_thread, &pthread2))  
    {  
        log_err("Can't create OBD thread!");  
        return 1;  
    } 
#endif

	worker1 = awoke_worker_create("obd_read", 2, 
		WORKER_FEAT_PERIODICITY|WORKER_FEAT_SUSPEND, read_obd_thread);
	worker2 = awoke_worker_create("eng_read", 200, 
		WORKER_FEAT_PERIODICITY|WORKER_FEAT_SUSPEND|WORKER_FEAT_CUSTOM_DEFINE, 
		read_eng_thread);
	//awoke_worker_start(worker1);
	//awoke_worker_start(worker2);

	fd = awoke_socket_server(SOCK_LOCAL, SOCK_ADDR, 0, FALSE);
	if (fd < 0) {
		log_err("create local server error");
	}

	lsr.name = awoke_string_dup("test_server");
	lsr.fd = fd;
	event = &lsr.event;
	event->fd = fd;
	event->type = EVENT_LISTENER;
	event->mask = EVENT_EMPTY;
	event->status = EVENT_NONE;

	server_start(&lsr);

	close(fd);
}

void client_run()
{
	/*
	int connect_fd;
	int ret;
	char snd_buf[1024];
	char recv_buff[1024];
	int i;
	static struct sockaddr_un srv_addr;
	connect_fd=socket(PF_UNIX,SOCK_STREAM,0);
	if(connect_fd<0)
	{
		perror("cannot create communication socket");
		return 1;
	}	
	srv_addr.sun_family=AF_UNIX;
	strcpy(srv_addr.sun_path,SOCK_ADDR);
	ret=connect(connect_fd,(struct sockaddr*)&srv_addr,sizeof(srv_addr));
	if(ret==-1)
	{
		perror("cannot connect to the server");
		close(connect_fd);
		return 1;
	}
	memset(snd_buf,0,1024);
	strcpy(snd_buf,"hypnos wakeup");
	write(connect_fd,snd_buf,sizeof(snd_buf));
	int rc = read(connect_fd, recv_buff, sizeof(recv_buff));
	printf("recv_buff %s\n", recv_buff);
	close(connect_fd);
	return 0;
	*/
	char snd_buf[1024];
	char rec_buf[1024];
	strcpy(snd_buf,"hypnos wakeup");
	int fd, rc;
	awoke_unix_message_send(SOCK_ADDR, snd_buf, strlen(snd_buf), &fd, UNIX_SOCK_F_WAIT_REPLY);
	err_type ret = awoke_socket_recv_wait(fd, 3000);
	if (et_ok != ret) {
		return ret;
	}
	rc = read(fd, rec_buf, 1024);
	printf("recv_buff %s\n", rec_buf);
	close(fd);
	return 0;
}

int main(int argc, char **argv)
{
	//server_run();
	client_run();

	return 0;
}
