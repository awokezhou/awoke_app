#ifndef __AWOKE_MEMORY_H__
#define __AWOKE_MEMORY_H__

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/sysinfo.h>

#include "awoke_macros.h"
#include "awoke_log.h"



static inline void *mem_alloc(const size_t size)
{
    void *aux = malloc(size);

    if (awoke_unlikely(!aux && size)) {
        return NULL;
    }

    return aux;
}

static uint32_t free_heap(void)
{
	int ret;
	struct sysinfo info;

	ret = sysinfo(&info);
	if (ret < 0) {
		log_err("get sysinfo error:%d", ret);
		return ret;
	}

	return info.freeram;
}

static inline void *mem_alloc_trace(const size_t size, const char *reason)
{
	void *aux;

	if (size) {

		log_trace("%s size:%lu free:%lu", reason, size, free_heap());
	
		aux = malloc(size);

		if (awoke_unlikely(!aux && size)) {
			log_trace("%s alloc error", reason);
        	return NULL;
   		}

		return aux;
	}

	return NULL;
}

static inline void *mem_alloc_z(const size_t size)
{

    void *buf = calloc(1, size);

    if (awoke_unlikely(!buf)) {
        return NULL;
    }

    return buf;
}

static inline void *mem_realloc(void *ptr, const size_t size)
{
    void *aux = realloc(ptr, size);

    if (awoke_unlikely(!aux && size)) {
        return NULL;
    }

    return aux;
}

static inline void mem_free(void *ptr)
{
    free(ptr);
}

typedef struct
{
    const char *p;
    unsigned long len;
} mem_ptr_t;

#define mem_ptr_none()	{NULL,0}

#define mem_ptr_dump(ptr) (ptr.len, ptr.p)

static inline mem_ptr_t mem_mk_ptr(const char *str)
{
	mem_ptr_t ret = {str, 0};

	if (str != NULL) ret.len = strlen(str);

	return ret;
}

static inline mem_ptr_t mem_mkptr_num(int num)
{
	char num_str[16] = {0x0};
	sprintf(num_str, "%d", num);
	return mem_mk_ptr(num_str);
}

static inline void mem_ptr_copy(mem_ptr_t *dst, mem_ptr_t *src)
{
	dst->p = src->p;
	dst->len = src->len;
}

#define mem_ptr_prfmt		"%.*s"
#define mem_ptr_print(ptr)	(ptr.len, ptr.p)

#endif /* __AWOKE_MEMORY_H__ */

