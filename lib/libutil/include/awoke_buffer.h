
#ifndef __AWOKE_BUFFER_H__
#define __AWOKE_BUFFER_H__



#include "awoke_type.h"
#include "awoke_list.h"
#include "awoke_error.h"
#include "awoke_macros.h"
#include "awoke_memory.h"
#include "awoke_random.h"

#define AWOKE_BUFFCHUNK_VERSION		"0.0.1"

//#define AWOKE_BUFFCHUNK_FIXED

/* -- buffchunk --{ */

#define AWOKE_BUFFCHUNK_FIXED_SIZE	1024
#define AWOKE_BUFFCHUNK_SIZE		4096
#define AWOKE_BUFFCHUNK_LIMIT		AWOKE_BUFFCHUNK_SIZE*2
#define AWOKE_BUFFCHUNK_POOL_LIMIT	AWOKE_BUFFCHUNK_SIZE*16

#define AWOKE_BUFFCHUNK_TRYBEST_TRIP	512

typedef struct _awoke_buffchunk {
	uint16_t id;
	char *p;
	int size;		/* memory size */
	int length;		/* data size */
#ifdef AWOKE_BUFFCHUNK_FIXED
	char _fixed[AWOKE_BUFFCHUNK_FIXED_SIZE];
#endif
	awoke_list _head;
} awoke_buffchunk;

typedef struct _awoke_buffchunk_pool {
	int size;		/* memory size */
	int length;		/* data size */
	int maxsize;	/* limit size */
	int chunknr;
	awoke_list chunklist;
} awoke_buffchunk_pool;
/* }-- buffchunk -- */



/* -- pubic interface -- { */
bool awoke_buffchunk_dynamic(struct _awoke_buffchunk *chunk);
struct _awoke_buffchunk *awoke_buffchunk_create(int size);
err_type awoke_buffchunk_init(struct _awoke_buffchunk *chunk);
void awoke_buffchunk_clean(struct _awoke_buffchunk *chunk);
void awoke_buffchunk_free(struct _awoke_buffchunk **p_chunk);
err_type awoke_buffchunk_write(struct _awoke_buffchunk *chunk, const char *data, 
	int length, bool trybest);
err_type awoke_buffchunk_resize(struct _awoke_buffchunk *chunk, 
	int resize, bool trybest);
const char *awoke_buffchunk_version();
int awoke_buffchunk_sizelimit();
int awoke_buffchunk_remain(struct _awoke_buffchunk *chunk);
void awoke_buffchunk_dump(struct _awoke_buffchunk *chunk);
struct _awoke_buffchunk_pool *awoke_buffchunk_pool_create(int maxsize);
err_type awoke_buffchunk_pool_init(struct _awoke_buffchunk_pool *pool, int maxsize);
void awoke_buffchunk_pool_clean(struct _awoke_buffchunk_pool *pool);
void awoke_buffchunk_pool_free(struct _awoke_buffchunk_pool **p_pool);
void awoke_buffchunk_pool_dump(struct _awoke_buffchunk_pool *pool);
err_type awoke_buffchunk_pool_chunkadd(struct _awoke_buffchunk_pool *pool, 
	struct _awoke_buffchunk *chunk);
struct _awoke_buffchunk *awoke_buffchunk_pool2chunk(struct _awoke_buffchunk_pool *pool);
/* }-- public interface -- */


#endif /* __AWOKE_BUFFER_H__ */

