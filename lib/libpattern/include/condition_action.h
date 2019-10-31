
#ifndef __CONDITION_ACTION_H__	
#define __CONDITION_ACTION_H__

#include "awoke_log.h"
#include "awoke_type.h"
#include "awoke_list.h"
#include "awoke_error.h"
#include "awoke_memory.h"

typedef err_type (*action_fn)(void *);
typedef struct _action {
	const char *name;
	uint16_t type;
	action_fn fn;
	uint32_t priv;
	awoke_list _head;
	awoke_list _regi;
} action;

typedef struct _condition {
	const char *name;
	uint8_t type;
	awoke_list _head;
	awoke_list _action_list;
} condition;

typedef struct _condition_action {
	awoke_list _ca[2];
} condition_action;


#define cond_list_get(ca)	&ca->_ca[0]
#define actn_list_get(ca)	&ca->_ca[1]

void condition_init(condition_action *ca);
err_type action_register(condition_action *ca, action *a);
err_type condition_register(condition_action *ca, condition *c);
err_type condition_action_add(condition_action *ca, uint8_t ctype, uint16_t atype);
err_type condition_tigger(condition_action *ca, uint8_t type, void *data);
#endif /* __CONDITION_ACTION_H__ */
