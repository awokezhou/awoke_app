#ifndef __AWOKE_RTMSG_H__
#define __AWOKE_RTMSG_H__

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>

#include "awoke_type.h"
#include "awoke_error.h"
#include "awoke_string.h"
#include "awoke_memory.h"
#include "awoke_macros.h"


#define RT_TM_ASSOCIATE_MSG_RECV			2000

#define RTMSG_SOCK_ADDR				"/var/unix_sock_addr_rtmsg"

typedef enum {
	rtmsg_associate 				= 0x13000001,
	rtmsg_disassociate 				= 0x13000002,
	rtmsg_client_online				= 0x13000021,
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
		     f_response,
		     f_broadcast;

	uint32_t word;
	uint32_t data_len;

	struct _rtmsg_header *next;
}rtmsg_header;

typedef struct _rtmsg_app_info {
	rt_app_id rid;
	const char *name;
	uint32_t flags;
#define RA_F_NULL	0x00000000
#define RA_F_BCAST	0x00000001		/* can use broadcast 	*/
#define RA_F_NORSD	0x00000002		/* not resident 		*/
}rtmsg_app_info;

typedef struct _msg_client_online {
	rt_app_id rid;
	char name[32];
}msg_client_online;


#define rt_app_foreach(head, size, info)	\
	int __i;								\
	info = &head[0];						\
											\
	for (__i = 0;							\
		 __i < size;						\
		 __i++, 							\
		 	info = &head[__i])				\

rtmsg_app_info *get_app_array();
int get_app_array_size();

#endif /* __AWOKE_RTMSG_H__ */
