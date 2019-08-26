#ifndef __AWOKE_STRING_H__
#define __AWOKE_STRING_H__


#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>


typedef struct _build_ptr {
	char *p;
	char *head;
	int max;
	unsigned long len;
} build_ptr;

#define build_ptr_none()	{NULL, NULL, 0, 0}

#define build_ptr_init(s, max)	{(s), (s), max, strlen(s)}

static inline build_ptr build_ptr_make(char *s, int max)
{
	build_ptr ptr;

	ptr.p = s;
	ptr.head = s;
	ptr.max = max;
	ptr.len = strlen(s);
	return ptr;
}

static inline int __build_ptr(build_ptr *ptr, const char *fmt, ...)
{
	int len;
	va_list ap;
	
	va_start(ap, fmt);
	if (ptr->max <= 0)
		return -1;
	len = vsnprintf(ptr->p, ptr->max, fmt, ap);
	ptr->max -= len;
	va_end(ap);

	ptr->len += len;
	ptr->p += len;
	
	return len;		
}

#define build_ptr_string(p, s)	(__build_ptr(&p, "%s", s))
#define build_ptr_number(p, n)	(__build_ptr(&p, "%d", n))
#define build_ptr_hex(p, h)		(__build_ptr(&p, "%x", h))



char *awoke_string_dup(const char *s);


#endif /* __AWOKE_STRING_H__ */
