#ifndef __POSITIONER_H__
#define __POSITIONER_H__

#include "awoke_event.h"
#include "position_policy.h"

typedef struct _hqnb_object {
    char *name;
    struct _hqnb_object *father;
    struct _hqnb_object *child;  
} hqnb_object;

typedef struct _hqnb_positioner {

    hqnb_object _object;

    hqnb_position_policy policy;

    awoke_list resources;

    awoke_event_loop evloop;
    
    char *position_data_ptr;

} hqnb_positioner;


#endif /* __POSITIONER_H__ */