/* 
 * This file is originally (C) Linus Torvalds.
 * It was modified by Prelude developers.
 */

#ifndef _LIBPRELUDE_LIST_H
#define _LIBPRELUDE_LIST_H

#include "prelude-inttypes.h"


/*
 * Simple doubly linked list implementation.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */

typedef struct prelude_list {
	struct prelude_list *next, *prev;
} prelude_list_t;


#define PRELUDE_LIST(name) \
	prelude_list_t name = { &(name), &(name) }



static inline void prelude_list_init(prelude_list_t *ptr) 
{
        ptr->next = ptr;
        ptr->prev = ptr;
}



/*
 * Insert a new entry between two known consecutive entries. 
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __prelude_list_add(prelude_list_t *new, prelude_list_t *prev, prelude_list_t *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}



/**
 * prelude_list_add:
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void prelude_list_add(prelude_list_t *head, prelude_list_t *new)
{
	__prelude_list_add(new, head, head->next);
}



/**
 * prelude_list_add_tail:
 * @new: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void prelude_list_add_tail(prelude_list_t *head, prelude_list_t *new)
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
static inline void __prelude_list_del(prelude_list_t *prev, prelude_list_t *next)
{
	next->prev = prev;
	prev->next = next;
}



/**
 * prelude_list_del:
 * @entry: the element to delete from the list.
 *
 * Deletes entry from list.
 * 
 * Note: prelude_list_empty on entry does not return true after this,
 *       the entry is in an undefined state.
 */
static inline void prelude_list_del(prelude_list_t *entry)
{
	__prelude_list_del(entry->prev, entry->next);
}



/**
 * prelude_list_del_init:
 * @entry: the element to delete from the list.
 *
 * Deletes entry from the list and reinitialize it.
 */
static inline void prelude_list_del_init(prelude_list_t *entry)
{
	__prelude_list_del(entry->prev, entry->next);
	prelude_list_init(entry); 
}



/**
 * prelude_list_empty:
 * @head: the list to test.
 *
 * Tests whether a list is empty.
 *
 * Returns: TRUE if the list is empty, FALSE otherwise.
 */
static inline prelude_bool_t prelude_list_is_empty(const prelude_list_t *head)
{
	return head->next == head;
}




/**
 * prelude_list_splice:
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * Join two lists.
 */
static inline void prelude_list_splice(prelude_list_t *head, prelude_list_t *list)
{
	prelude_list_t *first = list->next;

	if ( first != list ) {
		prelude_list_t *last = list->prev;
		prelude_list_t *at = head->next;

		first->prev = head;
		head->next = first;

		last->next = at;
		at->prev = last;
	}
}



/**
 * prelude_list_entry:
 * @ptr: Pointer to a #prelude_list_t entry.
 * @type: Type of the object @ptr is embedded in.
 * @member: Member name of the list object within @type.
 *
 * Get the object associated with @ptr.
 *
 * Returns: The object associated with @ptr.
 */
#define prelude_list_entry(ptr, type, member) \
	((type *)((unsigned long)(ptr) - (unsigned long)(&((type *)0)->member)))


#define prelude_list_for_each_continue_safe(head, pos, bkp)     \
        for ( (pos) = (((bkp) == NULL) ? (head)->next : (bkp)); \
              ((bkp) = (pos)->next), (pos) != (head);           \
              (pos) = (bkp))



#define prelude_list_for_each_continue(head, pos)                     \
        for ( (pos) = (((pos) == NULL) ? (head)->next : (pos)->next); \
              (pos) != (head);                                        \
              (pos) = (pos)->next)



/**
 * prelude_list_for_each:
 * @pos: Pointer to a #prelude_list_t to use as the iterator.
 * @head: Pointer to the head of your list.
 *
 * Iterate over a @headn from head to tail.
 * It is not safe to delete an entry from the loop.
 */
#define prelude_list_for_each(head, pos) \
	for (pos = (head)->next; pos != (head); pos = pos->next)




/**
 * prelude_list_for_each_safe:
 * @pos: Pointer to a #prelude_list_t to use as the iterator.
 * @n: Pointer to a #prelude_list_t to use as the backup for the next entry.
 * @head: Pointer to the head of your list.
 *
 * Iterate over a @head, from head to tail.
 * The next pointer is saved in @n, making it possible to call prelude_list_del()
 * on the current list entry.
 */
#define prelude_list_for_each_safe(head, pos, n) \
        for (pos = (head)->next, n = pos->next; pos != (head); \
                pos = n, n = pos->next)



/**
 * prelude_list_for_each_reversed:
 * @pos: Pointer to a #prelude_list_t to use as the iterator.
 * @head: Pointer to the head of your list.
 *
 * Iterate over @head entry, from tail to head.
 * It is not safe to delete an entry from the loop.
 */
#define prelude_list_for_each_reversed(head, pos) \
        for (pos = (head)->prev; pos != (head); pos = pos->prev)



/**
 * prelude_list_for_each_reversed_safe:
 * @pos: Pointer to a #prelude_list_t to use as the iterator.
 * @n: Pointer to a #prelude_list_t to use as the backup for the next entry.
 * @head: Pointer to the head of your list.
 *
 * Iterate over a @head, from tail to head.
 * The next pointer is saved in @n, making it possible to call prelude_list_del()
 * on the current list entry.
 */

#define prelude_list_get_next(list, pos, class, member) \
        pos ? \
                ((pos)->member.next == (list)) ? NULL : \
                                prelude_list_entry((pos)->member.next, class, member) \
        : \
                ((list)->next == (list)) ? NULL : \
                                prelude_list_entry((list)->next, class, member)


#define prelude_list_get_next_safe(list, pos, bkp, class, member)                                                                \
        pos ?                                                                                                            \
              (((pos) = bkp),                                                                                            \
               ((bkp) = (! (bkp) || (bkp)->member.next == list) ? NULL : prelude_list_entry((pos)->member.next, class, member)), \
               (pos))                                                                                                    \
        :                                                                                                                \
              (((pos) = ((list)->next == list) ? NULL : prelude_list_entry((list)->next, class, member)),                        \
               ((bkp) = (! (pos) ||(pos)->member.next == list ) ? NULL : prelude_list_entry((pos)->member.next, class, member)), \
               (pos))

#endif /* _LIBPRELUDE_LIST_H */
