
#ifndef __BK_FILECACHE_H__
#define __BK_FILECACHE_H__



#include "benchmark.h"



/*
 * FileCache item {
 */
typedef struct _bk_filecache_item {
	int id;
	awoke_list _head;	
} bk_filecache_item;
/*
 * FileCache item }
 */


/*
 * FileCache {
 */
typedef struct _bk_filecache {

	char *name;

	int maxsize;

	int cachemax;

	int cachenum;
	
	awoke_list caches;
	
	uint8_t init_finish:1;
	
} bk_filecache;
/*
 * FileCache }
 */



err_type benchmark_filecache_test(int argc, char *argv[]);

#endif /* __BK_FILECACHE_H__ */