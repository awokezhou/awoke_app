#ifndef __AWOKE_TYPE_H__
#define __AWOKE_TYPE_H__

#include <stdlib.h>

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef TRUE
#define TRUE (1)
#endif

typedef unsigned char _bool;
#ifndef bool
#define bool _bool
#endif

#ifndef int8_t
typedef char _int8_t;
#define int8_t _int8_t
#endif

#ifndef int16_t
typedef short int _int16_t;
#define int16_t _int16_t
#endif

#ifndef int32_t
typedef int _int32_t;
#define int32_t _int32_t
#endif

#ifndef int64_t
typedef long _int64_t;
#define int64_t _int64_t
#endif

#ifndef uint8_t
typedef unsigned char _uint8_t;
#define uint8_t _uint8_t
#endif

#ifndef uint16_t
typedef unsigned short int _uint16_t;
#define uint16_t _uint16_t
#endif

#ifndef uint32_t
typedef unsigned int _uint32_t;
#define uint32_t _uint32_t
#endif

#ifndef uint64_t
typedef unsigned long _uint64_t;
#define uint64_t _uint64_t
#endif



#endif /* __AWOKE_TYPE_H__ */
