
#ifndef __PROTO_XFER_H__
#define __PROTO_XFER_H__


#include "awoke_type.h"
#include "awoke_error.h"
#include "awoke_buffer.h"
#include "cmder_protocol.h"
#include "cmder.h"

#define XFER_PREFIX			0xAA
#define XFER_POSFIX			0xFF

#define XFER_CMD_RD			0x01
#define XFER_CMD_WR			0x02
#define XFER_CMD_CL			0x03

#define XFER_SPACE_BIT 		3	
#define XFER_SPACE_SHIFT 	29
#define XFER_SPACE_MASK 	(uint32_t)(((1<<XFER_SPACE_BIT)-1)<<XFER_SPACE_SHIFT)

#define XFER_OFFSET_BIT 	(32-XFER_SPACE_BIT)
#define XFER_OFFSET_MASK 	((uint32_t)~XFER_SPACE_MASK)

#define XFER_SPACE_MCU    	(0)
#define XFER_SPACE_FPGA   	(1)
#define XFER_SPACE_SENSOR 	(2)
#define XFER_SPACE_FLASH    (3)
#define XFER_SPACE_VAM      (4)

#define XFER_MAXPAYLOAD 	256

#define __XFER_GET_SPACE(x)    	((uint32_t)(x&XFER_SPACE_MASK))
#define __XFER_GET_OFFSET(x)    ((uint32_t)(x&XFER_OFFSET_MASK))
#define XFER_GET_SPACE(cmd)    (__XFER_GET_SPACE(((struct xfer_head *)cmd)->addr))
#define XFER_GET_OFFSET(cmd)   (__XFER_GET_OFFSET(((struct xfer_head *)cmd)->addr))
#define XFER_GET_DATA(cmd)     (((uint8_t *)cmd)+sizeof(struct xfer_head))

#define XFER_FLASH_OUTRANGE			0xFFFF
#define XFER_FLASH_OUTRANGE_SIZE	64*1024		/* 4K */

#define XFER_VAM_OFFSET_BPC     0x00000000      /* size:256MB */
#define XFER_VAM_OFFSET_NUC     0x10000000      /* size:256MB */
#define XFER_VAM_OFFSET_SPC     0x40000000      /* size:256MB */

#define XFER_MCU_PARAMS_BASE    0x00001000
#define XFER_MCU_ALGORITHM_BASE 0x00020000

typedef enum {
    XFER_ADDR_RANGE_BOOTINFO = 0x01,
    XFER_ADDR_RANGE_PARAMS = 0x02,
    XFER_ADDR_RANGE_ALGORITHM = 0x3,
} xfer_addr_range_e;

typedef enum {
    XFER_ADDR_BITWIDTH =        0x00001000,
    XFER_ADDR_FPS =             0x00001010,
    XFER_ADDR_FPS_MIN =         0x00001014,
    XFER_ADDR_FPS_MAX =         0x00001018,
    XFER_ADDR_TRIGGER =         0x00001030,
    XFER_ADDR_INVS_EN =         0x00001040,
    XFER_ADDR_HINVS =           0x00001044,
    XFER_ADDR_VINVS =           0x00001048,
    XFER_ADDR_CROSS_EN =        0x00001050,
    XFER_ADDR_CROSS_X =         0x00001054,
    XFER_ADDR_CROSS_Y =         0x00001058,
    XFER_ADDR_EXPO =            0x00001060,
    XFER_ADDR_EXPO_MIN =        0x00001064,
    XFER_ADDR_EXPO_MAX =        0x00001068,
    XFER_ADDR_EXPO_STEP =       0x0000106C,
    XFER_ADDR_GAIN =            0x00001070,
    XFER_ADDR_GAIN_MIN =        0x00001074,
    XFER_ADDR_GAIN_MAX =        0x00001078,
    XFER_ADDR_GAIN_STEP =       0x0000107C,
    XFER_ADDR_CONFIG_SAVE =     0x00002000,
    XFER_ADDR_CONFIG_RESTORE =  0x00002004,
    XFER_ADDR_ORIGIN_IMAGE =    0x00020000,
    XFER_ADDR_COLOR_MODE =      0x00020004,
    XFER_ADDR_BLC =             0x00030000,
    XFER_ADDR_DPC =             0x00031000,
    XFER_ADDR_SPC =             0x00032000,
    XFER_ADDR_NUC =             0x00033000,
    XFER_ADDR_GAMMA =           0x00034000,
    XFER_ADDR_IFFR =            0x00035000,
    XFER_ADDR_BINING =          0x00036000,
} xfer_mcu_address_e;

struct xfer_head {
	uint8_t mark;
	uint8_t cmd:2;
	uint8_t seq:2;
    uint8_t space:4;
	uint16_t len;
	uint32_t addr;
};

struct xfer_mcu_req {
	uint32_t address;
	err_type (*set)(struct uartcmder *ctx, struct cmder_protocol *proto, 
        uint8_t *in, int length);
	err_type (*get)(struct uartcmder *ctx, struct cmder_protocol *proto, struct xfer_head *head, 
        awoke_buffchunk *rsp);
};

struct xfer_tail {
	uint8_t code;
	uint8_t mark;   //0xff
};


extern struct cmder_protocol xfer_protocol;

#endif /* __XFER_H__ */

