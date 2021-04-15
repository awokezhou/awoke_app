
#include "awoke_tree.h"
#include "awoke_log.h"



void awoke_btree_init(awoke_btree *t, int (*compare)(const void *, const void *))
{
	t->root = NULL;
	t->compare = compare;
}

static err_type btree_add(awoke_btree *t, awoke_btree_node *r, awoke_btree_node *n)
{
	int c;
	err_type ret = et_ok;

	c = t->compare(n->data, r->data);
	if (c < 0) {
		if (!r->left) {
			r->left = n;
		} else {
			ret = btree_add(t, r->left, n);
		}
	} else if (c > 0) {
		if (!r->right) {
			r->right = n;
		} else {
			ret = btree_add(t, r->right, n);
		}
	}

	return ret;
}

err_type awoke_btree_add(awoke_btree *t, awoke_btree_node *n)
{	
	err_type ret = et_ok;

	if (!t || !n || !t->compare)
		return et_param;

	if (!t->root) {
		t->root = n;
	} else {
		ret = btree_add(t, t->root, n);
	}

	return ret;
}

static void awoke_btree_inorder(awoke_btree_node *n, void (*pfn)(void *))
{
	if (n) {
		awoke_btree_inorder(n->left, pfn);
		(*pfn)(n->data);
		awoke_btree_inorder(n->right, pfn);
	}
}

void awoke_btree_traverse(awoke_btree *t, void (*pfn)(void *))
{
	if (t) {
		awoke_btree_inorder(t->root, pfn);
	}
}

typedef struct _awoke_btree_testnode_t {
	int data;
	awoke_btree_node btn;
} awoke_btree_testnode_t;

void awoke_btree_test_dump(void *data)
{
	awoke_btree_testnode_t *n = (awoke_btree_testnode_t *)data;
	log_trace("n data:%d", n->data);
}

void awoke_btree_test(void)
{
	awoke_btree tree;
	awoke_btree_testnode_t n1, n2, n3, n4;

	n1.data = 1;
	n1.btn.data = &n1;

	n2.data = 2; 
	n2.btn.data = &n2;
	
	n3.data = 3;
	n3.btn.data = &n3;
	
	n4.data = 4;
	n4.btn.data = &n4;

	awoke_btree_init(&tree, NULL);

	awoke_btree_add(&tree, &n1.btn);
	awoke_btree_add(&tree, &n2.btn);

	awoke_btree_traverse(&tree, awoke_btree_test_dump);
}
