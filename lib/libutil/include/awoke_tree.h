
#ifndef __AWOKE_TREE_H__
#define __AWOKE_TREE_H__



#include "awoke_type.h"
#include "awoke_error.h"



typedef struct _awoke_btree_node {
	struct _awoke_btree *left;
	struct _awoke_btree *right;
	void *data;
} awoke_btree_node;

typedef struct _awoke_btree {
	struct _awoke_btree_node *root;
	int (*compare)(const void *, const void *);
} awoke_btree;


void awoke_btree_init(awoke_btree *t, int (*compare)(const void *, const void *));
void awoke_btree_traverse(awoke_btree *t, void (*pfn)(void *));
err_type awoke_btree_add(awoke_btree *t, awoke_btree_node *n);

#endif /* __AWOKE_TREE_H__ */