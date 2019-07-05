#ifndef __AWOKE_NETWORK_H__
#define __AWOKE_NETWORK_H__

#include "awoke_type.h"

typedef struct _awoke_net_retrans {
	bool enable; 
	int max_nr;
	uint32_t counter;
#define RT_F_NONE	0x0000
#define RT_F_RECVCB	0x0001
#define RT_F_ACTIVE	0x0010
	uint16_t flag;
} awoke_net_retrans;


#endif /* __AWOKE_NETWORK_H__ */
