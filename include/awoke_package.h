#ifndef __AWOKE_PACKAGE_H__
#define __AWOKE_PACKAGE_H__

#include "awoke_type.h"


typedef struct _pkg_header_example {
	uint8_t type;
	uint8_t index;
	uint8_t version;
	uint8_t device;
	uint32_t time;
	uint8_t imei[8];
} pkg_header_ex;

#define pkt_dump(data, len) do {\
		int __i;\
		for (__i=0; __i<len; __i++) {\
			if (!(__i%16)) {\
				printf("\n");\
			}\
			printf("0x%x ", (uint8_t)data[__i]);\
		}\
		printf("\n");\
	}while(0)
	

#endif /* __AWOKE_PACKAGE_H__ */
