

#ifndef __QE_SERVICE_H__
#define __QE_SERVICE_H__


#include "qe_def.h"


/**
 * qe_container_of - return the member address of ptr, if the type of ptr is the
 * struct type.
 */
#define qe_container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - (unsigned long)(&((type *)0)->member)))

/**
 * @brief initialize a list object
 */
#define QE_LIST_OBJECT_INIT(object) { &(object), &(object) }

/**
 * @brief initialize a list
 *
 * @param l list to be initialized
 */
static inline void qe_list_init(qe_list_t *list)
{
    list->next = list->prev = list;
}

/** Return true if the list is empty.
 *
 * @param head pointer to the head of the list.
 */
static inline int qe_list_isempty(const qe_list_t *head)
{
	return ((head->next == head) && (head->prev == head));
}

/** add a new entry after an existing list element
 *
 * @param new       new entry to be added
 * @param existing  list element to add the new entry after.  This could
 *                  be the list head or it can be any element in the list.
 *
 */
static inline void qe_list_append(qe_list_t *new, qe_list_t *existing)
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
 *                  be the list head or it can be any element in the list.
 *
 */
static inline void qe_list_prepend(qe_list_t *new, qe_list_t *existing)
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
 * @param entry existing list entry to be unlinked from the list.
 */
static inline void qe_list_unlink(qe_list_t *entry)
{
   	entry->next->prev = entry->prev;
   	entry->prev->next = entry->next;

	entry->next = 0;
	entry->prev = 0;
}

static inline unsigned int qe_list_len(const qe_list_t *l)
{
    unsigned int len = 0;
    const qe_list_t *p = l;
    while (p->next != l)
    {
        p = p->next;
        len ++;
    }

    return len;
}

/**
 * @brief get the struct for this entry
 * @param node the entry point
 * @param type the type of structure
 * @param member the name of list in structure
 */
#define qe_list_entry(node, type, member) \
    qe_container_of(node, type, member)

/**
 * qe_list_foreach - iterate over a list
 * @pos:    the qe_list_t * to use as a loop cursor.
 * @head:   the head for your list.
 */
#define qe_list_foreach(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * qe_list_foreach_safe - iterate over a list safe against removal of list entry
 * @pos:	the qe_list_t * to use as a loop cursor.
 * @n:		another qe_list_t * to use as temporary storage
 * @head:	the head for your list.
 */
#define qe_list_foreach_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

/**
 * qe_list_for_each_entry  -   iterate over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member: the name of the list_struct within the struct.
 */
#define qe_list_foreach_entry(pos, head, member) \
		for (pos = qe_list_entry((head)->next, typeof(*pos), member); \
			 &pos->member != (head); \
			 pos = qe_list_entry(pos->member.next, typeof(*pos), member))

/**
 * qe_list_for_each_entry_safe - iterate over list of given type safe against removal of list entry
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member: the name of the list_struct within the struct.
 */
#define qe_list_foreach_entry_safe(pos, n, head, member) \
			for (pos = qe_list_entry((head)->next, typeof(*pos), member), \
				 n = qe_list_entry(pos->member.next, typeof(*pos), member); \
				 &pos->member != (head); \
				 pos = n, n = qe_list_entry(n->member.next, typeof(*n), member))

/**
 * qe_list_first_entry - get the first element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member: the name of the list_struct within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define qe_list_first_entry(ptr, type, member) \
				qe_list_entry((ptr)->next, type, member)
			
#define QE_SLIST_OBJECT_INIT(object) { QE_NULL }



#endif /* SRC_QE_SERVICE_H_ */


