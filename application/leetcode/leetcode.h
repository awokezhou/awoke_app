
#ifndef __LEETCODE_H__
#define __LEETCODE_H__


#include "awoke_log.h"
#include "awoke_type.h"
#include "awoke_error.h"
#include "awoke_list.h"
#include "awoke_macros.h"

typedef enum {
	ARG_NONE = 0,
	ARG_SL_LIST_REVERSE,		/* singly linked list reverse */
	ARG_SL_LIST_SWAP,			/* Swap Nodes in Pairs */
} arg_opt;


typedef struct _testdata {
	int x;
	struct _sl_list _head;
} testdata;



#endif /* __LEETCODE_H__ */
