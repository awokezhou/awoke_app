#ifndef __ROUTER_MAIN_H__
#define __ROUTER_MAIN_H__


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

#include "router_forward.h"

typedef struct _rt_param {
	int log_level;		/* log level 		 */
	int log_mode;		/* log mode 		 */
	bool daemon;      	/* run as daemon     */
}rt_param;

typedef struct _rt_manager {
	awoke_list conn_list;
	awoke_list forward_list;
	awoke_list listener_list;
	rt_param *param;
}rt_manager;

typedef struct _router_listener {
	char *name;
	char *address;
	uint16_t port;
	uint8_t type;
	int fd;
	awoke_event event;
	awoke_list _head;
}router_listener;

typedef struct _router_conn {
	awoke_event event;
	router_listener *listener;
	int status;                         /* connection status            */
#define CONN_OPEN		0x01
#define CONN_CLOSE		0x02
	awoke_list _head;
}router_conn;

#endif /* __ROUTER_MAIN_H__ */

