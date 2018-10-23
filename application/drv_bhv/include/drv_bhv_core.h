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
#include <sys/sem.h>



#include "awoke_type.h"
#include "awoke_error.h"
#include "awoke_memory.h"
#include "awoke_list.h"
#include "awoke_macros.h"
#include "awoke_log.h"
#include "awoke_vector.h"
#include "awoke_string.h"


#define SCHEDULER_CREATE_JOINABLE	PTHREAD_CREATE_JOINABLE
#define bhv_scheduler_id pthread_t
#define bhv_scheduler_attr pthread_attr_t
#define bhv_scheduler_attr_init(attr) pthread_attr_init(attr)
#define bhv_scheduler_attr_setdetachstate(attr, stat) pthread_attr_setdetachstate(attr, stat)
#define bhv_scheduler_create(id, attr, handler, context) pthread_create(id, attr, handler, context)
#define scheduler_sleep(tick) usleep(tick)

typedef struct _bhv_param {
	int dlevel;			/* debug level 		 */
	bool daemon;            /* run as daemon     */
}bhv_param;

typedef struct _bhv_scheduler {
	uint32_t tick;
#define TICK_FAST 10000
#define TICK_NORMAL 100000
#define TICK_SLOW 1000000	
	void (*scheduler)(void *);
	bool loop;
#define LOCK_KEY_ID 0x1335
	int sem_id;
}bhv_scheduler;

typedef struct _bhv_detection {
	bool if_crash;
	bool if_rp_acc;
	bool if_rp_dec;
	bool if_brakes;
	bool if_roll_over;
	bool if_sudden_turn;
}bhv_detection;

typedef struct _bhv_manager {
	struct _bhv_param *param;
	struct _bhv_detection detection;
	struct _bhv_scheduler scheduler;
	awoke_list worker_list;
	awoke_list calc_list;
	awoke_list dtct_list;
}bhv_manager;


typedef struct _bhv_worker {
	char *name;
	err_type (*worker_handler) (struct _bhv_worker *, struct _bhv_manager *);
	awoke_list _head;
	uint32_t calc_flag;			/* worker's calculate flag 				*/
	uint32_t tick;				
	uint32_t tick_int;			/* scheduler interval of worker 		*/
	uint32_t tick_val;			/* scheduler init value of worker 		*/
	awoke_list calculate_list;
}bhv_worker;

typedef struct _bhv_scheduler_context {
	struct _bhv_manager *manager;
}bhv_scheduler_context;

union semun
{
	int val;
	struct semid_ds *buf;
	unsigned short *arry;
};		


#endif /* __DRV_BHV_CORE_H__ */

