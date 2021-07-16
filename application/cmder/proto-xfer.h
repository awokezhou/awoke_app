
#ifndef __PROTO_XFER_H__
#define __PROTO_XFER_H__


#include "awoke_type.h"
#include "awoke_error.h"


#define XFER_MARK		0x55
#define XFER_POSTFIX	0x56

typedef enum {
	XFER_CT_GET = 0x0,
	XFER_CT_SET = 0x1,
	XFER_CT_ERASE = 0x2,
} xfer_ctype_e;

struct xferinfo {
	uint8_t ctype;
	uint8_t length;
	uint32_t address;
};



extern struct cmder_protocol xfer_protocol;



#endif /* __PROTO_XFER_H__ */
