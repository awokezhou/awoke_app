
#ifndef __GPS_OPTIMIZE_H__
#define __GPS_OPTIMIZE_H__

#include "awoke_type.h"
#include "awoke_error.h"
#include "awoke_queue.h"
#include "awoke_worker.h"



typedef struct _gps_point {
    bool is_infp;
    bool history;
    time_t time;
    uint8_t data;
} gps_point;

typedef struct _gps_history {
    bool locked;
    int select_nr;
    awoke_queue *q_cache;
} gps_history;

typedef struct _gps_optimizer {
    bool geninfp;
    gps_history history;
    awoke_queue *q_store;
    awoke_worker *parser;
    awoke_worker *poller;
    time_t store_time;
} gps_optimizer;


#endif /* __GPS_INFPCOMP_H__ */