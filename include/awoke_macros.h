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

#define array_size(array) (sizeof(array)/sizeof(array[0]))

#define mask_exst(mask, flag)   ((mask) & (flag))
#define mask_push(mask, flag)   ((mask) |= (flag))
#define mask_pull(mask, flag)	((mask) &= (~flag))
#define mask_only(mask, flag)   (!((mask) & (~(flag))))

#define buf_dump(buf, len) do {\
		int __i;\
		for (__i=0; __i<len; __i++) {\
			if (!(__i%16) && __i>1) {\
				printf("\n");\
			}\
			printf("%2x ", buf[__i]);\
		}\
		printf("\n");\
	}while(0)

#define _buf_push(data, p, bytes) do {\
					memcpy(p, (uint8_t *)&data, bytes);\
					p+= bytes;\
				}while(0)
#define buf_push_byte(data, p) 	_buf_push(data, p, 1)
#define buf_push_word(data, p) 	_buf_push(data, p, 2)
#define buf_push_dwrd(data, p) 	_buf_push(data, p, 4)

#define _buf_push_safe(data, p, len, end) do {\
		if ((end-p+1) >= len)\
			memcpy(p, (uint8_t *)&data, len);\
		else if ((end-p+1) == 0)\
			memcpy(p, (uint8_t *)&data, len);\
		p+= len;\
	}while(0)
#define buf_push_byte_safe(data, p, end) 		_buf_push_safe(data, p, 1, 	end)
#define buf_push_word_safe(data, p, end) 		_buf_push_safe(data, p, 2, 	end)
#define buf_push_dwrd_safe(data, p, end) 		_buf_push_safe(data, p, 4, 	end)
#define buf_push_stri_safe(data, p, end, ln)	_buf_push_safe(data, p, ln, end)

#define array_size(array)	((sizeof(array))/(sizeof(array[0])))

#define array_foreach(head, size, p)				\
		int __i;									\
		p = &head[0];								\
													\
		for (__i = 0;								\
			 __i < size;							\
			 __i++, 								\
				p = &head[__i]) 					\

#define print_ip_format "%d.%d.%d.%d"
#define print_ip(addr)	\
				addr[0], addr[1], addr[2], addr[3]


#endif /* __AWOKE_MACROS_H__ */

