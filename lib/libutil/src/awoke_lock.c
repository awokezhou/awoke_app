
#include "awoke_lock.h"


static void sem_acquire(awoke_lock *lock);
static void sem_release(awoke_lock *lock);
static void smt_destroy(awoke_lock *lock);
static err_type sem_create(awoke_lock *lock);
static err_type sem_acquire_timeout(awoke_lock *lock, uint32_t timeout);
static void mux_acquire(awoke_lock *lock);
static void mux_release(awoke_lock *lock);
static void mux_destroy(awoke_lock *lock);
static err_type mux_create(awoke_lock *lock);
static err_type mux_acquire_timeout(awoke_lock *lock, uint32_t timeout);


static awoke_lock_map lock_table[] = {
	{AWOKE_LOCK_T_SEM, sem_create, sem_acquire, sem_acquire_timeout, sem_release, smt_destroy},
	{AWOKE_LOCK_T_MUX, mux_create, mux_acquire, mux_acquire_timeout, mux_release, mux_destroy},
};

static void set_abswaittime(struct timespec *outtime, int ms)
{
	long sec;
	long usec;
	struct timeval tnow;
	gettimeofday(&tnow, NULL);
	usec = tnow.tv_usec + ms*1000;
	sec = tnow.tv_sec+usec/1000000;
	outtime->tv_nsec = (usec%1000000)*1000;
	outtime->tv_sec = sec;
}

static err_type sem_create(awoke_lock *lock)
{
	int ret;
	lock->_lock = mem_alloc_z(sizeof(sem_t));
	if (!lock->_lock)
		return et_nomem;

	ret = sem_init(lock->_lock, 0, 1);
	if (ret < 0)
		return et_lock_init;

	return et_ok;
}

static void sem_acquire(awoke_lock *lock)
{
	sem_t *s = (sem_t *)lock->_lock;
	sem_wait(s);
}

static err_type sem_acquire_timeout(awoke_lock *lock, uint32_t timeout)
{
	int ret;
	struct timespec ts;
	sem_t *s = (sem_t *)lock->_lock;

	set_abswaittime(&ts, timeout);

	ret = sem_timedwait(s, &ts);
	if (ret != 0) {
		return et_lock_timeout;
	}

	return et_ok;
}

static void sem_release(awoke_lock *lock)
{
	sem_t *s = (sem_t *)lock->_lock;
	sem_post(s);	
}

static void smt_destroy(awoke_lock *lock)
{
	sem_t *s = (sem_t *)lock->_lock;
	sem_destroy(s);
}

static err_type mux_create(awoke_lock *lock)
{
	int ret;
	lock->_lock = mem_alloc_z(sizeof(pthread_mutex_t));
	if (!lock->_lock)
		return et_nomem;

	ret = pthread_mutex_init(lock->_lock, NULL);
	if (ret < 0)
		return et_lock_init;

	return et_ok;
}

static void mux_acquire(awoke_lock *lock)
{
	pthread_mutex_t *m = (pthread_mutex_t *)lock->_lock;
	pthread_mutex_lock(m);
}

static err_type mux_acquire_timeout(awoke_lock *lock, uint32_t timeout)
{
	int ret;
	struct timespec ts;
	pthread_mutex_t *m = (pthread_mutex_t *)lock->_lock;

	set_abswaittime(&ts, timeout);
	
	ret = pthread_mutex_timedlock(m, &ts);
	if (ret != 0) {
		return et_lock_timeout;
	}
	return et_ok;
}

static void mux_release(awoke_lock *lock)
{
	pthread_mutex_t *m = (pthread_mutex_t *)lock->_lock;
	pthread_mutex_unlock(m);
}

static void mux_destroy(awoke_lock *lock)
{
	pthread_mutex_t *m = (pthread_mutex_t *)lock->_lock;
	pthread_mutex_destroy(m);
}

static void _lock_acquire(awoke_lock *lock)
{
	awoke_lock_map *p;
	awoke_lock_map *head = lock_table;
	int size = array_size(lock_table);

	array_foreach(head, size, p) {
		if ((p->type == lock->type) && (p->acquire != NULL)) {
			return p->acquire(lock);
		}
	}		
}

err_type awoke_lock_acquire(awoke_lock *lock, uint8_t flag)
{
	if ((lock->locked) && (flag == AWOKE_LOCK_F_NOBLOCK))
		return et_locked;
	
	_lock_acquire(lock);

	lock->locked = TRUE;

	return et_ok;
}

static err_type _lock_acquire_timeout(awoke_lock *lock, uint32_t timeout)
{
	awoke_lock_map *p;
	awoke_lock_map *head = lock_table;
	int size = array_size(lock_table);

	array_foreach(head, size, p) {
		if ((p->type == lock->type) && (p->acquire_timeout != NULL)) {
			return p->acquire_timeout(lock, timeout);
		}
	}	

	return et_fail;
}

err_type awoke_lock_acquire_timeout(awoke_lock *lock, uint32_t timeout)
{
	err_type ret;
		
	ret = _lock_acquire_timeout(lock, timeout);
	if (ret != et_ok) {
		return ret;
	}

	lock->locked = TRUE;
	return et_ok;
}

static void _lock_release(awoke_lock *lock)
{
	awoke_lock_map *p;
	awoke_lock_map *head = lock_table;
	int size = array_size(lock_table);

	array_foreach(head, size, p) {
		if ((p->type == lock->type) && (p->release != NULL)) {
			return p->release(lock);
		}
	}
}

void awoke_lock_release(awoke_lock *lock)
{
	_lock_release(lock);
	lock->locked = FALSE;
}

static void _lock_destroy(awoke_lock *lock)
{
	awoke_lock_map *p;
	awoke_lock_map *head = lock_table;
	int size = array_size(lock_table);

	array_foreach(head, size, p) {
		if ((p->type == lock->type) && (p->destroy != NULL)) {
			return p->destroy(lock);
		}
	}
}

void awoke_lock_destroy(awoke_lock *lock)
{
	return _lock_destroy(lock);
}

void awoke_lock_clean(awoke_lock **lock)
{
	awoke_lock *p;

	if (!lock || !*lock)
		return;

	p = *lock;

	if (p->_lock) {
		if (p->locked)
			_lock_release(p);
		_lock_destroy(p);
		mem_free(p->_lock);
	}

	mem_free(p);
	p = NULL;
}

static err_type _lock_create(awoke_lock *lock)
{
	awoke_lock_map *p;
	awoke_lock_map *head = lock_table;
	int size = array_size(lock_table);

	array_foreach(head, size, p) {
		if ((p->type == lock->type) && (p->create != NULL)) {
			return p->create(lock);
		}
	}

	return et_fail;
}

awoke_lock *awoke_lock_create(uint8_t type)
{
	err_type ret;
	awoke_lock *lock;

	lock = mem_alloc_z(sizeof(awoke_lock));
	if (!lock) 
		return NULL;

	lock->type = type;
	lock->locked = FALSE;
	
	ret = _lock_create(lock);
	if (ret != et_ok)
		goto err;
	
	return lock;

err:
	awoke_lock_clean(&lock);
	return NULL;
}
