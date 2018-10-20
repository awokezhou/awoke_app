#ifndef __AWOKE_MEMORY_H__
#define __AWOKE_MEMORY_H__

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "awoke_macros.h"


static inline void *mem_alloc(const size_t size)
{
    void *aux = malloc(size);

    if (awoke_unlikely(!aux && size)) {
        return NULL;
    }

    return aux;
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
    char *data;
    unsigned long len;
} mem_ptr_t;

static inline void mem_ptr_reset(mem_ptr_t * p)
{
    p->data = NULL;
    p->len = 0;
}

#endif /* __AWOKE_MEMORY_H__ */
