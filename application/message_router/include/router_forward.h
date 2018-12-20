#ifndef __ROUTER_FORWARD_H__
#define __ROUTER_FORWARD_H__


#include "router_main.h"


typedef enum
{
    app_not_run 				= 0,  		/**< not running */
    app_launched 				= 1,     	/**< launched, but waiting for confirmation */
    app_running  				= 2,      	/**< fully up and running. */
    app_termination_request		= 3 		/**< Requested termination, but waiting for confirmation. */
} app_stat;

typedef struct _forward_item {
	int fd;
	app_stat status;
	const rtmsg_app_info * rt_info;
	rtmsg_header *queue; 					/**< queue of messages waiting to be delivered to this app. */
	awoke_list _head;
}forward_item;


forward_item *forward_item_get_byrid(rt_app_id, awoke_list *);


#endif /* __ROUTER_FORWARD_H__ */

