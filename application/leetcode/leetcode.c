
#include <getopt.h>

#include "leetcode.h"
#include "link_list.h"

static inline struct _list_node *list_node_revert(struct _list_node *head)
{
	struct _list_node *p_prev=head, *p, *p_next;

	if (!head || !head->next)
		return head;
	
	p = head->next;
	p_prev->next = NULL;
	
	while (p != NULL) {
		p_next = p->next;
		p->next = p_prev;
		p_prev = p;
		p = p_next;
	}

	return p_prev;
}

static inline struct _list_node *list_node_swap(struct _list_node *head)
{
	struct _list_node *p_prev=NULL, *p1=head, *p2, *p_next;

	if (!head || !head->next)
		return head;

	p2 = p1->next;
	head = p2;

	while (p1 != NULL) {
		log_debug("loop");
		p_next = p2->next;
		p2->next = p1;
		p1->next = NULL;
		if (p_prev != NULL)
			p_prev->next = p2;
		p_prev = p1;
		if (!p_next)
			break;
		p1 = p_next;
		p2 = p1->next;
		if (!p2) {
			p_prev->next = p1;
			break;
		}
	}

	return head;
}

err_type link_list_reverse_test()
{
	list_node *p, *reverse;
	list_node head, node1, node2, node3, node4;	
	
	list_node_init(&head, 1);

	node1.x = 2;
	node2.x = 3;
	node3.x = 4;
	node4.x = 5;

	list_node_append(&node1, &head);
	list_node_append(&node2, &head);
	list_node_append(&node3, &head);
	list_node_append(&node4, &head);

	list_node_foreach(p, &head) {
		log_debug("data:%d", p->x);
	}

	reverse = list_node_swap(&head);

	list_node_foreach(p, reverse) {
		log_debug("data:%d", p->x);
	}	

	return et_ok;
}

err_type link_list_swap_test()
{
	list_node *p, *swap;
	list_node head, node1, node2, node3, node4;	
	
	list_node_init(&head, 1);

	node1.x = 2;
	node2.x = 3;
	node3.x = 4;
	node4.x = 5;

	list_node_append(&node1, &head);
	list_node_append(&node2, &head);
	list_node_append(&node3, &head);
	list_node_append(&node4, &head);

	list_node_foreach(p, &head) {
		log_debug("data:%d", p->x);
	}

	swap = list_node_swap(&head);

	list_node_foreach(p, swap) {
		log_debug("data:%d", p->x);
	}	

	return et_ok;	
}

void usage(int rc)
{
	printf("usage : leetcode [option]\n\n");

    printf("    --reverse-link-list\t\tReverse Link List\n");
    printf("    --swap-link-list\t\tSwap Link List\n");

	EXIT(rc);	
}

int main(int argc, char *argv)
{
	int opt;
	err_type ret;
	err_type (*testcode)();

	log_level(LOG_DBG);
	log_mode(LOG_TEST);
	
	static const struct option long_opts[] = {
		{"reverse-link-list",		no_argument, 	NULL, 	ARG_SL_LIST_REVERSE},
		{"swap-link-list",			no_argument,	NULL,	ARG_SL_LIST_SWAP},
		{NULL, 0, NULL, 0}
	};

	while ((opt = getopt_long(argc, argv, "?h", long_opts, NULL)) != -1)
	{
		switch (opt) 
		{
			case ARG_SL_LIST_REVERSE:
				testcode = link_list_reverse_test;
				break;
			case ARG_SL_LIST_SWAP:
				testcode = link_list_swap_test;
				break;
			case '?':
			case 'h':
			default:
				usage(AWOKE_EXIT_SUCCESS);
		}
	}

	ret = testcode();

	return 0;
}
