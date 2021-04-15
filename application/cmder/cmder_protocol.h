
#ifndef __CMDER_PROTOCOL_H__
#define __CMDER_PROTOCOL_H__



#include "awoke_type.h"
#include "awoke_error.h"
#include "awoke_list.h"



struct cmder_protocol {
	char *name;
	
	err_type (*callback)(struct cmder_protocol *proto, uint8_t reason,
		void *user, void *in, unsigned int len);

	err_type (*connect)(void);
	err_type (*match)(struct cmder_protocol *, void *, int);
	err_type (*read)(struct cmder_protocol *, void *, int);
	err_type (*write)(struct cmder_protocol *, void *, int);
	err_type (*close)(struct cmder_protocol *, void *);
	
	void *private;
	void *context;
	
	awoke_list _head;
};


#endif /* __CMDER_PROTOCOL_H__ */
