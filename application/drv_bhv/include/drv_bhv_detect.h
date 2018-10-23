#ifndef __DRV_BHV_DETECT_H__
#define __DRV_BHV_DETECT_H__


#include "drv_bhv_core.h"

typedef struct _bhv_dtct {
	char *name;
	err_type (*dtct_handler) (struct _bhv_calc *, 
						      struct _bhv_dtct *, 
						      struct _bhv_manager *);
	uint32_t mask;
#define DTCT_NULL			0x0000
#define DTCT_CRASH			0x0001
#define DTCT_RP_ACC			0x0002
#define DTCT_RP_DEC			0x0004
#define DTCT_BRAKE			0x0008
#define DTCT_SUDDEN_TURN	0x0010
#define DTCT_ROLL_OVER		0x0020
	awoke_list _head;
}bhv_dtct;


err_type bhv_detect(struct _bhv_manager *manager);

#endif /* __DRV_BHV_DETECT_H__ */
