
#ifndef __AWOKE_LOCK_H__
#define __AWOKE_LOCK_H__


#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include<sys/time.h>

#include "awoke_log.h"
#include "awoke_type.h"
#include "awoke_error.h"
#include "awoke_memory.h"


#define AWOKE_LOCK_F_NOBLOCK	0x01
#define AWOKE_LOCK_F_BLOCK		0x02


typedef struct _awoke_lock {
#define AWOKE_LOCK_T_SEM	0x01
#define AWOKE_LOCK_T_MUX	0x02
	uint8_t type;
	bool locked;
	void *_lock;
} awoke_lock;

typedef struct _awoke_lock_map {
	uint8_t type;
	err_type (*create)(awoke_lock *);
	void (*acquire)(awoke_lock *);
	err_type (*acquire_timeout)(awoke_lock *, uint32_t);
	void (*release)(awoke_lock *);
	void (*destroy)(awoke_lock *);
} awoke_lock_map;


awoke_lock *awoke_lock_create(uint8_t type);
void awoke_lock_clean(awoke_lock **lock);
void awoke_lock_release(awoke_lock *lock);
void awoke_lock_destroy(awoke_lock *lock);
err_type awoke_lock_acquire_timeout(awoke_lock *lock, uint32_t timeout);
err_type awoke_lock_acquire(awoke_lock *lock, uint8_t flag);

#endif /* __AWOKE_LOCK_H__ */
