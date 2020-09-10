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

	
#define pkg_dump(data, len) do {\
					int __i;\
					for (__i=0; __i<len; __i++) {\
						if (!(__i%16)) {\
							printf("\n");\
						}\
						printf("0x%2x ", data[__i]);\
					}\
					printf("\n");\
				}while(0)
			
			
#define _pkg_push_bytes(data, p, bytes) do {\
					memcpy(p, (char *)&data, bytes);\
					p+= bytes;\
				}while(0)
#define _pkg_push_stris(data, p, bytes) do {\
					strncpy(p, (char *)&data, bytes);\
					p+= bytes;\
				}while(0)
#define pkg_push_byte(data, p) 		_pkg_push_bytes(data, p, 1)
#define pkg_push_word(data, p) 		_pkg_push_bytes(data, p, 2)
#define pkg_push_dwrd(data, p) 		_pkg_push_bytes(data, p, 4)
#define pkg_push_bytes(data, p, len) 		_pkg_push_bytes(data, p, len)
#define pkg_push_stris(data, p, len)		_pkg_push_stris(data, p, len)
			
#define _pkg_push_bytes_safe(data, p, bytes, end) do {\
					if ((end - p) >= bytes)\
						memcpy(p, (unsigned char *)&data, bytes);\
					p+= bytes;\
				}while(0)
#define _pkg_push_stris_safe(data, p, bytes, end) do {\
					if ((end - p) >= bytes)\
						strncpy(p, data, bytes);\
					p+= bytes;\
				}while(0)

#define pkg_push_byte_safe(data, p, end) 	_pkg_push_bytes_safe(data, p, 1, end)
#define pkg_push_word_safe(data, p, end) 	_pkg_push_bytes_safe(data, p, 2, end)
#define pkg_push_dwrd_safe(data, p, end) 	_pkg_push_bytes_safe(data, p, 4, end)
#define pkg_push_bytes_safe(data, p, end, ln)	_pkg_push_bytes_safe(data, p, ln, end)
#define pkg_push_stris_safe(data, p, end, ln)	_pkg_push_stris_safe(data, p, ln, end)

#define _pkg_bytes_pull(data, p, bytes) do {\
		memcpy((unsigned char *)&data, p, bytes);\
		p+= bytes;\
	}while(0)
#define _pkg_stris_pull(data, p, bytes) do {\
		strncpy(data, p, bytes);\
		p+= bytes;\
	}while(0)

#define pkg_pull_byte(data, p)		_pkg_bytes_pull(data, p, 1)
#define pkg_pull_word(data, p)		_pkg_bytes_pull(data, p, 2)
#define pkg_pull_dwrd(data, p)		_pkg_bytes_pull(data, p, 4)
#define pkg_pull_bytes(data, p, len)	_pkg_bytes_pull(data, p, len)
#define pkg_pull_stris(data, p, len)	_pkg_stris_pull(data, p, len)



uint16_t awoke_checksum_u16(void *data, int len);
uint16_t awoke_crc16(uint8_t *data, int len);


#endif /* __AWOKE_PACKAGE_H__ */
