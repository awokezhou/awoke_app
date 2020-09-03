
#ifndef __REACTOR_H__
#define __REACTOR_H__



#include "awoke_type.h"
#include "awoke_error.h"
#include "awoke_worker.h"



err_type reactor_worker(void *ctx);
err_type handshark_msg_send(void);


#endif /* __REACTOR_H__ */
