#ifndef __POSITION_POLICY_H__
#define __POSITION_POLICY_H__

#include "awoke_type.h"
#include "awoke_list.h"

typedef struct _hqnb_position_policy_entry {

#define POSITION_PID_1  1
    uint8_t id;
    uint32_t store_freq;
    uint32_t send_freq;
    uint32_t send_num;
} hqnb_position_policy_entry;

typedef struct _hqnb_position_policy {
    uint8_t using;
    hqnb_position_policy_entry *policy_table;
    hqnb_position_policy_entry *policy_size;
} hqnb_position_policy;


#endif /* __POSITION_POLICY_H__ */