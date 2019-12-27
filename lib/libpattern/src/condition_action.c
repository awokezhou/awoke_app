
#include "condition_action.h"

void condition_init(condition_action *ca)
{
	awoke_list *cond_list = cond_list_get(ca);
	awoke_list *actn_list = actn_list_get(ca);
	list_init(cond_list);
	list_init(actn_list);
}

void condition_clean(condition_action *ca)
{
	action *actn, *temp;
	condition *cond;
	awoke_list *clist = cond_list_get(ca);

	list_for_each_entry(cond, clist, _head) {
		list_for_each_entry_safe(actn, temp, &cond->_action_list, _head) {
			list_unlink(&actn->_head);
			mem_free(actn);
		}
	}
}

err_type condition_register(condition_action *ca, condition *c)
{
	awoke_list *p = &ca->_ca[0];
	list_init(&c->_action_list);
	list_append(&c->_head, p);
	return et_ok;
}

err_type action_register(condition_action *ca, action *a)
{
	awoke_list *p = &ca->_ca[1];
	list_append(&a->_regi, p);
	return et_ok;
}

static err_type _cond_action_add(condition *c, action *a)
{
	list_append(&a->_head, &c->_action_list);
	return et_ok;
}

err_type condition_action_add(condition_action *ca, uint8_t ctype, uint16_t atype)
{
	bool find_action = FALSE;
	bool find_condtion = FALSE;
	action *actn, *add = NULL;
	condition *cond;
	awoke_list *cond_list = &ca->_ca[0];
	awoke_list *actn_list = &ca->_ca[1];

	list_for_each_entry(cond, cond_list, _head) {
		if (cond->type == ctype) {
			find_condtion = TRUE;
			break;
		}
	}

	list_for_each_entry(actn, actn_list, _regi) {
		if (actn->type == atype) {
			find_action = TRUE;
			add = mem_alloc_z(sizeof(struct _action));
			if (!add)
				return et_nomem;
			memcpy(add, actn, sizeof(struct _action));
			break;
		}
	}

	if (find_action && find_condtion) {
		return _cond_action_add(cond, add);
	}

	if (add)
		mem_free(add);

	return et_fail;
}

static err_type tigger_action(condition *cond, void *data)
{
	err_type ret;
	action *actn;
	
	list_for_each_entry(actn, &cond->_action_list, _head) {
		
		log_debug("action:%s", actn->name);

		/* call action */
		if (actn->fn != NULL) {
			ret = actn->fn(data);
			if (ret != et_ok) {
				log_err("action %s error, ret %d", ret);
				return ret;
			}
		}
	}

	return et_ok;
}

err_type condition_tigger(condition_action *ca, uint8_t type, void *data)
{
	condition *cond;
	awoke_list *cond_list = cond_list_get(ca);

	list_for_each_entry(cond, cond_list, _head) {
		if (cond->type == type) {
			log_debug("trigger condition:%s", cond->name);
			return tigger_action(cond, data);
		}
	}

	return et_fail;
}

