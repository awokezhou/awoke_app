#ifndef __DRV_BHV_CORE_H__
#define __DRV_BHV_CORE_H__

#include <stdio.h>
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


#include "awoke_type.h"
#include "awoke_error.h"
#include "awoke_memory.h"
#include "awoke_list.h"
#include "awoke_macros.h"
#include "awoke_log.h"
#include "awoke_vector.h"

#define CALC_NULL			0x0000
#define CALC_VEC_ACC		0x0001
#define CALC_VEC_CORNER		0x0002
#define CALC_ACC_ANGLE		0x0004
#define CALC_GYR_ANGLE		0x0008
#define CALC_GPS_ACC		0x0010
#define CALC_GPS_CORNER		0x0020


#define SCHEDULER_CREATE_JOINABLE	PTHREAD_CREATE_JOINABLE
#define bhv_scheduler_id pthread_t
#define bhv_scheduler_attr pthread_attr_t
#define bhv_scheduler_attr_init(attr) pthread_attr_init(attr)
#define bhv_scheduler_attr_setdetachstate(attr, stat) pthread_attr_setdetachstate(attr, stat)
#define bhv_scheduler_create(id, attr, handler, context) pthread_create(id, attr, handler, context)
#define scheduler_sleep(tick) usleep(tick)

typedef struct _bhv_param {
	uint8_t dlevel;			/* debug level 		 */
	bool daemon;            /* run as daemon     */
};
typedef struct _bhv_param bhv_param;


typedef struct _bhv_scheduler {
	uint32_t tick;
#define TICK_FAST 10000
#define TICK_NORMAL 100000
#define TICK_SLOW 1000000	
	void (*scheduler)(struct _bhv_manager *);
	bool loop;
};
typedef struct _bhv_scheduler bhv_scheduler;

typedef struct _bhv_worker {
	char *name;
	err_type (*worker_handler)(struct _bhv_manager *);
	awoke_list _head;
	uint32_t calc_flag;			/* worker's calculate flag 				*/
	uint32_t tick;				
	uint32_t tick_int;			/* scheduler interval of worker 		*/
	uint32_t tick_val;			/* scheduler init value of worker 		*/
	awoke_list calculate_list;
};
typedef struct _bhv_worker bhv_worker;

typedef struct _bhv_scheduler_context {
	struct _bhv_manager *manager;
};
typedef struct _bhv_scheduler_context bhv_scheduler_context;


typedef struct _bhv_manager {
	struct _bhv_param *param;
	struct _bhv_scheduler scheduler;
	awoke_list worker_list;
};
typedef struct _bhv_manager bhv_manager;


#endif /* __DRV_BHV_CORE_H__ */

