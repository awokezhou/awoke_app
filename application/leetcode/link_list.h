
#ifndef __LINK_LIST_H__
#define __LINK_LIST_H__


typedef struct _list_node {
	int x;
	struct _list_node *next;
} list_node;

static inline bool list_node_empty(struct _list_node *head)
{
	return (!head->next);
}

static inline void list_node_init(struct _list_node *head, int x)
{
	head->x = x;
	head->next = NULL;
}

static inline struct _list_node *list_node_tail(struct _list_node *head)
{
	struct _list_node *p = head;

	while (p->next != NULL) {
		p = p->next;
	}

	return p;
}

static inline void list_node_append(struct _list_node *new, struct _list_node *head)
{
	struct _list_node *tail;
	
	tail = list_node_tail(head);
	tail->next = new;
	new->next = NULL;
}

#define list_node_foreach(pos, head)		\
	for (pos = (head);						\
		 pos != NULL;						\
		 pos = pos->next)					\


#endif /* __LINK_LIST_H__ */

