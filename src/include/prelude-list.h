/* 
   This file is originally (C) Linus Torvalds.
   Slight changes were made by Prelude developers.
*/ 

#ifndef _LIBPRELUDE_LIST_H
#define _LIBPRELUDE_LIST_H

/* 
 * NOTE: This file must be always included AFTER <sys/queue.h> (or, any file
 * including <sys/queue.h>); otherwise LIST_HEAD macro will be defined
 * to three-argument version from <sys/queue.h> which is probably not what
 * you want. If you include this file after <sys/queue.h> it will
 * redefine the macro to version used in Prelude. 
 */

#ifdef LIST_HEAD
#undef LIST_HEAD
#endif /* LIST_HEAD */

/*
 * Simple doubly linked list implementation.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */

typedef struct prelude_list_head {
	struct prelude_list_head *next, *prev;
} prelude_list_t;


#define PRELUDE_LIST_HEAD_INIT(name) { &(name), &(name) }

#define PRELUDE_LIST_HEAD(name) \
	prelude_list_t name = PRELUDE_LIST_HEAD_INIT(name)

#define PRELUDE_INIT_LIST_HEAD(ptr) do { \
	(ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

/*
 * Insert a new entry between two known consecutive entries. 
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __prelude_list_add(prelude_list_t * new,
	prelude_list_t * prev,
	prelude_list_t * next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

/**
 * prelude_list_add - add a new entry
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void prelude_list_add(prelude_list_t *new, prelude_list_t *head)
{
	__prelude_list_add(new, head, head->next);
}

/**
 * prelude_list_add_tail - add a new entry
 * @new: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void prelude_list_add_tail(prelude_list_t *new, prelude_list_t *head)
{
	__prelude_list_add(new, head->prev, head);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __prelude_list_del(prelude_list_t * prev,
				  prelude_list_t * next)
{
	next->prev = prev;
	prev->next = next;
}

/**
 * prelude_list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: prelude_list_empty on entry does not return true after this, the entry is in an undefined state.
 */
static inline void prelude_list_del(prelude_list_t *entry)
{
	__prelude_list_del(entry->prev, entry->next);
}

/**
 * prelude_list_del_init - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
static inline void prelude_list_del_init(prelude_list_t *entry)
{
	__prelude_list_del(entry->prev, entry->next);
	PRELUDE_INIT_LIST_HEAD(entry); 
}

/**
 * prelude_list_empty - tests whether a list is empty
 * @head: the list to test.
 */
static inline int prelude_list_empty(const prelude_list_t *head)
{
	return head->next == head;
}

/**
 * prelude_list_splice - join two lists
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static inline void prelude_list_splice(prelude_list_t *list, prelude_list_t *head)
{
	prelude_list_t *first = list->next;

	if (first != list) {
		prelude_list_t *last = list->prev;
		prelude_list_t *at = head->next;

		first->prev = head;
		head->next = first;

		last->next = at;
		at->prev = last;
	}
}

/**
 * prelude_list_entry - get the struct for this entry
 * @ptr:	the &prelude_list_t pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 */
#define prelude_list_entry(ptr, type, member) \
	((type *)((unsigned long)(ptr) - (unsigned long)(&((type *)0)->member)))



/**
 * list_get_next - get next element from the list
 * @pos:    a list iterator pointer (class *). 
            If NULL, will be set to the list head. 
 *          Will be set to NULL after iterating over last list member. 
 * @list:   a pointer to list head. (prelude_list_t *)
 * @class:  object type
 * @member: list member in object (usually 'list')
 */
#define prelude_list_get_next(pos, list, class, member) \
        pos ? \
                ((pos)->member.next == (list)) ? NULL : \
                                prelude_list_entry((pos)->member.next, class, member) \
        : \
                ((list)->next == (list)) ? NULL : \
                                prelude_list_entry((list)->next, class, member)


#define prelude_list_get_next_safe(pos, bkp, list, class, member)                                                                \
        pos ?                                                                                                            \
              (((pos) = bkp),                                                                                            \
               ((bkp) = (! (bkp) || (bkp)->member.next == list) ? NULL : prelude_list_entry((pos)->member.next, class, member)), \
               (pos))                                                                                                    \
        :                                                                                                                \
              (((pos) = ((list)->next == list) ? NULL : prelude_list_entry((list)->next, class, member)),                        \
               ((bkp) = (! (pos) ||(pos)->member.next == list ) ? NULL : prelude_list_entry((pos)->member.next, class, member)), \
               (pos))


/**
 * list_for_each	-	iterate over a list
 * @pos:	the &prelude_list_t to use as a loop counter.
 * @head:	the head for your list.
 */
#define prelude_list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

#define prelude_list_for_each_safe(pos, n, head) \
        for (pos = (head)->next, n = pos->next; pos != (head); \
                pos = n, n = pos->next)

#define prelude_list_for_each_reversed(pos, head) \
        for (pos = (head)->prev; pos != (head); pos = pos->prev)

#define prelude_list_for_each_reversed_safe(pos, n, head) \
        for (pos = (head)->prev, n = pos->prev; pos != (head); \
                 pos = n, n = pos->prev)

#endif /* _LIBPRELUDE_LIST_H */
