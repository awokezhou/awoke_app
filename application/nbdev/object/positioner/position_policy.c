#include "awoke_memory.h"
#include "awoke_list.h"
#include "awoke_lock.h"
#include "awoke_error.h"
#include "awoke_macros.h"
#include "position_policy.h"
#include "position_resource.h"


static hqnb_position_policy_entry g_policy_table[] = {
    {POSITION_PID_1, 5, 10, 25},
};

static int g_policy_table_size = array_size(g_policy_table);

static err_type resource_register(struct _hqnb_position_policy *policy, uint8_t type, 
    err_type (*start)(void), err_type (*stop)(void), err_type (*sleep)(void))
{
    hqnb_position_resource *res, *nres;

    list_for_each_entry(res, &policy->resources, _head) {
        if (res->type == type) {
            awoke_debug("resource[%d] exist", res->type);
            return et_exist;
        }
    }

    nres = awoke_mem_zalloc(sizeof(hqnb_position_resource));
    if (!nres) {
        awoke_err("alloc res error");
        return et_nomem;
    }

    nres->type = type;
    nres->start = start;
    nres->stop = stop;
    nres->sleep = sleep;
    nres->state = 0;

    list_append(&nres->_head, &policy->resources);
    return et_ok;
}

static err_type position_resource_init(struct _hqnb_position_policy *policy) 
{
    awoke_list_init(&policy->resources);

    resource_register(policy, POSITION_RES_GPS, NULL, NULL, NULL);
    resource_register(policy, POSITION_RES_WIFI, NULL, NULL, NULL);
    resource_register(policy, POSITION_RES_CELL, NULL, NULL, NULL);

    return et_ok;
}

err_type hqnb_position_policy_init(struct _hqnb_position_policy *policy, uint8_t using)
{
    position_resource_init(policy);

    policy->policy_table = g_policy_table;
    policy->policy_size = g_policy_table_size;
    policy->using = using;
    return et_ok;
}

err_type position_data_poll(struct _hqnb_positioner *positioner)
{
    hqnb_position_info *info = positioner->position_data_ptr;
    hqnb_position_policy *policy = &positioner->policy;

    if (position_can_store(policy)) {
        position_info_store(positioner);
    }
}

err_type hqnb_mainloop(struct _hqnb_positioner *positioner)
{
    /* some condition, resource start */
    if (1) {
        position_resource_start(positioner, POSITION_RES_GPS);
    } else {
        position_resource_stop(positioner, POSITION_RES_GPS);
    }

    position_data_poll(positioner);


}