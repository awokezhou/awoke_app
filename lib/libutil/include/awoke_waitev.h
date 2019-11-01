
#ifndef __AWOKE_WAIT_H__
#define __AWOKE_WAIT_H__

#include "awoke_type.h"
#include "awoke_error.h"
#include "awoke_macros.h"
#include "awoke_memory.h"
#include "awoke_string.h"

typedef struct _awoke_wait_ev {
	const char *name;
	err_type ret;
	int tick;
	int _reference;
#define WAIT_EV_F_SLEEP			0x01
#define WAIT_EV_F_CUSTOM_STOP	0x02
#define WAIT_EV_F_REFERENCE		0x03
	uint8_t flag;
	err_type (*run_once)(struct _awoke_wait_ev *);
	bool (*stop_fn)(struct _awoke_wait_ev *);
	void *data;
} awoke_wait_ev;


err_type awoke_wait_test1();
awoke_wait_ev *awoke_waitev_create(char *name, int tick, uint8_t flags, 
			err_type (*run_once)(struct _awoke_wait_ev),
			bool (*stop_fn)(struct _awoke_wait_ev), void *data);
err_type awoke_waitev(awoke_wait_ev *ev);
#endif /* __AWOKE_WAIT_H__ */
