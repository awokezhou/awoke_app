#ifndef __AWOKE_LIST_H__
#define __AWOKE_LIST_H__


/*! \brief structure that must be placed at the begining of any structure
 *         that is to be put into the linked list.
 */
typedef struct _awoke_list {
	struct _awoke_list *next;   /**< next pointer */
	struct _awoke_list *prev;   /**< previous pointer */
} awoke_list;


/** Initialize a field in a structure that is used as the head of a dlist */
#define LIST_HEAD_IN_STRUCT_INIT(field) do {\
      (field).next = &(field);               \
      (field).prev = &(field);               \
   } while (0)

/** Initialize a standalone variable that is the head of a dlist */
#define LIST_HEAD_INIT(name) { &(name), &(name) }

/** Declare a standalone variable that is the head of the dlist */
#define LIST_HEAD(name) \
	struct _awoke_list name = LIST_HEAD_INIT(name)


static inline void list_init(struct _awoke_list *list)
{
    list->next = list;
    list->prev = list;
}


/** Return true if the dlist is empty.
 *
 * @param head pointer to the head of the dlist.
 */
static inline int list_empty(const struct _awoke_list *head)
{
	return ((head->next == head) && (head->prev == head));
}


/** add a new entry after an existing list element
 *
 * @param new       new entry to be added
 * @param existing  list element to add the new entry after.  This could
 *                  be the list head or it can be any element in the dlist.
 *
 */
static inline void list_append(struct _awoke_list *new, struct _awoke_list *existing)
{
	existing->prev->next = new;
	
	new->next = existing;
	new->prev = existing->prev;
	
	existing->prev = new;
}


/** add a new entry in front of an existing list element
 *
 * @param new       new entry to be added
 * @param existing  list element to add the new entry in front of.  This could
 *                  be the list head or it can be any element in the dlist.
 *
 */
static inline void list_prepend(struct _awoke_list *new, struct _awoke_list *existing)
{
   existing->next->prev = new;

   new->next = existing->next;
   new->prev = existing;

   existing->next = new;
}


/** Unlink the specified entry from the list.
 *  This function does not free the entry.  Caller is responsible for
 *  doing that if applicable.
 *
 * @param entry existing dlist entry to be unlinked from the dlist.
 */
static inline void list_unlink(struct _awoke_list *entry)
{
   	entry->next->prev = entry->prev;
   	entry->prev->next = entry->next;

	entry->next = 0;
	entry->prev = 0;
}

#undef offsetof
#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)


#define container_of(ptr, type, member) ({			\
        const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
        (type *)( (char *)__mptr - offsetof(type,member) );})


#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

#define list_entry_first(ptr, type, member) container_of((ptr)->next, type, member)
#define dlist_entry_last(ptr, type, member) container_of((ptr)->prev, type, member)

#define list_for_each_entry(pos, head, member)				\
	for (pos = list_entry((head)->next, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = list_entry(pos->member.next, typeof(*pos), member))

#define list_for_each_entry_safe(pos, tmp, head, member)				\
        for (pos = list_entry((head)->next, typeof(*pos), member),\
             tmp = list_entry(pos->member.next, typeof(*pos), member); \
             &pos->member != (head);                    \
             pos = tmp, tmp = list_entry(pos->member.next, typeof(*pos), member))


typedef struct _sl_list {
	struct _sl_list *next;
} sl_list;

#define SL_LIST_INIT(name) {&name}

static inline void sl_list_init(struct _sl_list *list)
{
    list->next = NULL;
}

static inline int sl_list_empty(const struct _sl_list *head)
{
	return (!head->next);
}

static inline void sl_list_append(struct _sl_list *new, struct _sl_list *existing)
{
	sl_list *p = existing;

	while (p->next != NULL) {
		p = p->next;
	}

	p->next = new;
	new->next = NULL;
}

static inline int sl_list_size(struct _sl_list *existing)
{
	int count = 0;
	sl_list *p = existing->next;

	while (p != NULL) {
		p = p->next;
		count++;
	}

	return count;
}

static inline void sl_list_reverse(struct _sl_list *existing)
{
	struct _sl_list *p1, *p2;
	struct _sl_list *p = existing->next;
	struct _sl_list *head = existing;

	if (sl_list_size(existing) <= 2) {
		p->next->next = p;
		head->next = p->next;
		p->next = NULL;
		return;
	}

	p1 = p;
	p = p->next;
	p2 = p->next;
	p1->next = NULL;
	
	while(p2->next != NULL) {
		printf("x\n");
		p->next = p1;
		p1 = p;
		p = p2;
		p2 = p2->next;
	}

	p->next = p1;
	p2->next = p;
	head->next = p2;
}

static inline void sl2_list_reverse(struct _sl_list *existing)
{
	printf("sl_list_reverse\n");
	int i = 0;
	struct _sl_list *map[5];
	printf("0.1\n");
	int size = sl_list_size(existing);
	printf("0.2, size %d\n", size);
	struct _sl_list *p = existing->next;

	printf("1\n");

	while (p != NULL) {
		printf("1.1 %d\n", i);
		map[i] = p;
		p = p->next;
		i++;
	}

	printf("2\n");

	for (i=0; i<size-1; i++) {
		printf("2.1 %d\n", i);
		map[i+1]->next = map[i];
	}

	printf("3\n");
	
	map[0]->next = NULL;
	existing->next = map[size-1];
}

#define sl_list_foreach(pos, head, member)				\
	for (pos = list_entry((head)->next, typeof(*pos), member);	\
	     &pos->member.next != NULL; 					\
	     pos = list_entry(pos->member.next, typeof(*pos), member))




#endif  /*__AWOKE_LIST_H__ */

