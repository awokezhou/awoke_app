
#ifndef __AWOKE_CMDLINE_H__
#define __AWOKE_CMDLINE_H__



#include "awoke_type.h"
#include "awoke_error.h"
#include "awoke_macros.h"


#define AWOKE_CMDLINE_MAXLEN	80

typedef struct _cmdline_item {
	const char *name;
	const char *tips;
	const char *match;
	err_type (*action)(struct _cmdline_item *);
} awoke_cmdline_item;

typedef struct _awoke_cmdline {
	const char *description;
	int command_nr;
	struct _cmdline_item *items;
} awoke_cmdline;


err_type awoke_cmdline_read(char *buffer, int maxlen);
err_type awoke_cmdline_wait(struct _cmdline_item *command_table, int table_size);

#endif /* __AWOKE_CMDLINE_H__ */