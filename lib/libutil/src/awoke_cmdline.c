
#include <stdio.h>
#include <string.h>
#include "awoke_log.h"
#include "awoke_cmdline.h"



err_type awoke_cmdline_read(char *buffer, int maxlen)
{
	log_info("Please input command:");
	
	while (fgets(buffer, maxlen, stdin) != NULL) {

		log_trace("input:%s", buffer);
		return et_ok;
	}

	return et_fail;
}

awoke_cmdline *awoke_cmdline_get(struct _cmdline_item *command_table, int size)
{
	err_type ret;
	char command[AWOKE_CMDLINE_MAXLEN] = {'\0'};
	awoke_cmdline_item *head = command_table, *p;

	ret = awoke_cmdline_read(command, AWOKE_CMDLINE_MAXLEN);
	if (ret != et_ok) {
		log_err("read command error");
		return NULL;
	}

	array_foreach_start(head, size, p) {

		if (strstr(command, p->name)) {
			log_trace("find command %s", p->name);
			return p;
		}
	} array_foreach_end();

	return NULL;
}

err_type awoke_cmdline_run(struct _cmdline_item *c)
{
	if (!c) {
		return et_fail;
	}
	
	return c->action(c);
}

err_type awoke_cmdline_wait(struct _cmdline_item *command_table, int table_size)
{
	err_type ret;
	
	ret = awoke_cmdline_run(awoke_cmdline_get(command_table, table_size));

	return ret;
}
