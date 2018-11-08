#ifndef __AWOKE_RTMSG_H__
#define __AWOKE_RTMSG_H__


#include "awoke_type.h"




#define RTMSG_SOCK_ADDR				"/var/local_sock_addr_rtmsg"

typedef enum {
	rtmsg_associate 				= 0x13000001,
	rtmsg_disassociate 				= 0x13000002,
}rtmsg_type;

typedef enum {
	app_none = 0,
	app_message_router,
	app_test_server,
	app_test_client,
	app_max,
}rt_app_id;

typedef struct _rtmsg_handle {
	int fd;
	rt_app_id rid;
}rtmsg_handle;

typedef struct _rtmsg_header {

	rt_app_id src;

	rt_app_id dst;

	rtmsg_type msg_type;

	uint16_t f_event,
		     f_request,
		     f_response;

	uint32_t word;
	uint32_t data_len;
	
}rtmsg_header;


#endif /* __AWOKE_RTMSG_H__ */
