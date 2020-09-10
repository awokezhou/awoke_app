#ifndef __AWOKE_STRING_H__
#define __AWOKE_STRING_H__


#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "awoke_type.h"

typedef struct _build_ptr {
	char *p;
	char *head;
	int max;
	unsigned long len;
} build_ptr;

#define build_ptr_none()	{NULL, NULL, 0, 0}

#define build_ptr_init(s, max)	{(s), (s), max, 0}

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
	len = vsnprintf(ptr->p, ptr->max, fmt, ap);
	va_end(ap);	
	ptr->max -= len;
	ptr->p += len;
	ptr->len += len;
	return len;		
}

#define build_ptr_string(p, s)			(__build_ptr(&p, "%s", s))
#define build_ptr_number(p, n)			(__build_ptr(&p, "%d", n))
#define build_ptr_hex(p, h)				(__build_ptr(&p, "%x", h))
#define build_ptr_format(p, fmt, ...)	(__build_ptr(&p, fmt, __VA_ARGS__))


char *awoke_string_dup(const char *s);
int awoke_string_str2bcd(uint8_t *bin_data, const char *str_data, uint16_t bin_len);
int awoke_string_bcd2str(char *out, const uint8_t *in, uint16_t b_len);
int awoke_string_str2bcdv2(uint8_t *bin_data, const char *str_data, uint16_t bin_len);
int awoke_string_from_hex(const uint8_t *hex, char *string, uint16_t hex_len);


#endif /* __AWOKE_STRING_H__ */
