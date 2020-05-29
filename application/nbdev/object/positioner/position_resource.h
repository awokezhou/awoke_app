#ifndef __POSITION_RESOURCE_H__
#define __POSITION_RESOURCE_H__

#include "awoke_type.h"
#include "awoke_list.h"

typedef struct _hqnb_position_resource {

#define POSITION_RES_GPS    0x01
#define POSITION_RES_WIFI   0x02
#define POSITION_RES_CELL   0x03
    uint8_t type;

    uint8_t state;

    err_type (*start)(void);
    err_type (*stop)(void);
    err_type (*sleep)(void);
    
    awoke_list _head;
} hqnb_position_resource;


#endif /* __POSITION_RESOURCE_H__ */