#ifndef __AWOKE_MACROS_H__
#define __AWOKE_MACROS_H__

#include <stdlib.h>
#include <errno.h>

#define no_argument         0
#define required_argument   1
#define optional_argument   2


#define AWOKE_EXIT_SUCCESS    EXIT_SUCCESS
#define AWOKE_EXIT_FAILURE    EXIT_FAILURE
#define EXIT(r)         exit(r)

#ifdef __GNUC__
  #define awoke_unlikely(x) 		__builtin_expect((x),0)
  #define awoke_likely(x) 			__builtin_expect((x),1)
  #define awoke_prefetch(x, ...) 	__builtin_prefetch(x, __VA_ARGS__)
#else
  #define awoke_unlikely(x)      	(x)
  #define awoke_likely(x)        	(x)
  #define awoke_prefetch(x, ...) 	(x, __VA_ARGS__)
#endif

#define awoke_get_pagesize() 		sysconf(_SC_PAGESIZE)


#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define container_of(ptr, type, member) ({			\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})


#endif /* __AWOKE_MACROS_H__ */

