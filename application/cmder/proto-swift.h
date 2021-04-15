
#ifndef __PROTO_SWIFT_H__
#define __PROTO_SWIFT_H__



#include "cmder_protocol.h"


typedef enum {

	SWIFT_CALLBACK_PROTOCOL_INIT,
	SWIFT_CALLBACK_PROTOCOL_DESTROY,

	SWIFT_CALLBACK_CONNECTION_ESTABLISHED,
	SWIFT_CALLBACK_CONNECTION_RECEIVE,
	SWIFT_CALLBACK_CONNECTION_RECEIVE_TIMEOUT,
	SWIFT_CALLBACL_CONNECTION_WRITABLE,
	SWIFT_CALLBACK_CONNECTION_ERROR,
	SWIFT_CALLBACK_CONNECTION_CLOSED,

} swift_callback_reasons;


struct swift_connect {
	unsigned int f_connected:1;
};


extern struct cmder_protocol swift_protocol;

#endif /* __PROTO_SWIFT_H__ */

