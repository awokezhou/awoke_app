
#ifndef __REACTOR_H__
#define __REACTOR_H__



#include "awoke_type.h"
#include "awoke_error.h"
#include "awoke_worker.h"



err_type reactor_worker(void *data);
void destroy_message(void *_msg);


#endif /* __REACTOR_H__ */
