/*
 * qe_core.h
 *
 *  Created on: 2021Äê7ÔÂ19ÈÕ
 *      Author: z04200
 */

#ifndef __QE_CORE_H__
#define __QE_CORE_H__


#include "qe_config.h"
#include "qe_debug.h"
#include "qe_def.h"
#include "qe_service.h"



qe_err_t qe_device_register(struct qe_device *dev,
                                          const char *name,
                                          qe_uint16_t flags);
qe_err_t qe_device_unregister(struct qe_device *dev);
struct qe_device *qe_device_find(const char *name);
qe_err_t qe_device_control(qe_device_t dev, int cmd, void *arg);


/**
 * Memory Interface
 */
void  qe_free(void *ptr);
void *qe_malloc(qe_size_t size);
void *qe_zalloc(qe_size_t size);
void *qe_realloc(void *buf, qe_size_t size);
void  qe_heap_init(void *start, qe_size_t size);
void  qe_mem_set_eapi(void* (*malloc)(qe_size_t size), 
							    void* (*zalloc)(qe_size_t size),
							    void* (*realloc)(void *buf, qe_size_t size),
							    void  (*free)(void *ptr));

/**
 * Stage Initialization
 */
#define qe_si_declaration(stage)	void qe_##stage##_init(void)
qe_si_declaration(board);
qe_si_declaration(prev);
qe_si_declaration(device);
qe_si_declaration(component);
qe_si_declaration(env);
qe_si_declaration(application);
void qe_stage_initialize(void);



void qe_assert_set_hook(void (*hook)(const char *, const char *, qe_size_t));
void qe_assert_handler(const char *ex_string, const char *func, qe_size_t line);

#define qe_assert(ex)                                                      	\
if (!(ex))                                                               	\
{                                                                         	\
    qe_assert_handler(#ex, __FUNCTION__, __LINE__);                       	\
}

/**
 * @group Service @{
 */
char *qe_strncpy(char *dst, const char *src, qe_ubase_t n);
qe_int32_t qe_strncmp(const char *cs, const char *ct, qe_uint32_t count);
void *qe_memset(void *s, int c, qe_ubase_t count);
void *qe_memcpy(void *dst, void *src, qe_ubase_t count);

qe_err_t qe_get_errno(void);
void qe_set_errno(qe_err_t e);
int *_qe_errno(void);
#ifndef errno
#define errno	*_qe_errno()
#endif

qe_size_t qe_strlen(const char *s);
qe_int32_t qe_vsnprintf(char       *buf,
                        	 qe_size_t   size,
                        	 const char *fmt,
                        	 va_list     args);
qe_int32_t qe_snprintf(char *buf, qe_size_t size, const char *fmt, ...);


int qe_strb_build(qe_strb_t *bpr, const char *fmt, ...);

#define qe_strb_none()					{NULL, NULL, 0, 0}
#define qe_strb_init(s, max)			{(s), (s), max, qe_strlen(s)}

#define qe_strb_string(p, s)			(qe_strb_build(&p, "%s", s))
#define qe_strb_number(p, n)			(qe_strb_build(&p, "%d", n))
#define qe_strb_hex(p, h)				(qe_strb_build(&p, "%x", h))
#define qe_strb_format(p, fmt, ...)		(qe_strb_build(&p, fmt, ##__VA_ARGS__))

/**}@*/
#endif /* __QE_CORE_H__ */

