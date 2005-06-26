/*****
*
* Copyright (C) 2005 PreludeIDS Technologies. All Rights Reserved.
* Author: Yoann Vandoorselaere <yoann.v@prelude-ids.com>
*
* This file is part of the Prelude library.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2, or (at your option)
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; see the file COPYING.  If not, write to
* the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*
*****/

/*
 * This is a blind rewrite of the kernel linked list handling code,
 * done so that we can dual-license libprelude. The code still look
 * pretty similar, but there is no way to write such list implementation
 * in many different manner.
 */

#ifndef HAVE_LIBPRELUDE_PRELUDE_LIST_H
#define HAVE_LIBPRELUDE_PRELUDE_LIST_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "prelude-inttypes.h"

#ifdef __cplusplus
  extern "C" {
#endif

#define PRELUDE_LIST(item) prelude_list_t (item) = { &(item), &(item) }


typedef struct prelude_list {
        struct prelude_list *next;
        struct prelude_list *prev;
} prelude_list_t;



static inline void __prelude_list_add(prelude_list_t *item, prelude_list_t *prev, prelude_list_t *next)
{
        prev->next = item;
        item->prev = prev;
        item->next = next;
        next->prev = item;
}


/**
 * prelude_list_init:
 * @item: Pointer to a #prelude_list_t object.
 *
 * Initialize @item. Note that this is only required if @item
 * is the head of a list, but might also be useful in case you
 * want to use prelude_list_del_init().
 */
static inline void prelude_list_init(prelude_list_t *item)
{
        item->next = item->prev = item;
}



/**
 * prelude_list_is_empty:
 * @item: Pointer to a #prelude_list_t object.
 *
 * Check whether @item is empty or not.
 *
 * Returns: TRUE if @item is empty, FALSE otherwise.
 */
static inline prelude_bool_t prelude_list_is_empty(prelude_list_t *item)
{
        return (item->next == item) ? PRELUDE_BOOL_TRUE : PRELUDE_BOOL_FALSE;
}



/**
 * prelude_list_add:
 * @head: Pointer to a #prelude_list_t list.
 * @item: Pointer to a #prelude_list_t object to add to @head.
 *
 * Add @item at the beginning of @head list.
 */
static inline void prelude_list_add(prelude_list_t *head, prelude_list_t *item)
{
        __prelude_list_add(item, head, head->next);
}



/**
 * prelude_list_add_tail:
 * @head: Pointer to a #prelude_list_t list.
 * @item: Pointer to a #prelude_list_t object to add to @head.
 *
 * Add @item at the tail of @head list.
 */
static inline void prelude_list_add_tail(prelude_list_t *head, prelude_list_t *item)
{
        __prelude_list_add(item, head->prev, head);
}



/**
 * prelude_list_del:
 * @item: Pointer to a #prelude_list_t object.
 *
 * Delete @item from the list it is linked in.
 */
static inline void prelude_list_del(prelude_list_t *item)
{
        item->prev->next = item->next;
        item->next->prev = item->prev;
}



/**
 * prelude_list_del_init:
 * @item: Pointer to a #prelude_list_t object.
 *
 * Delete @item from the list it is linked in, and reinitialize
 * @item member so that the list can be considered as empty.
 */
static inline void prelude_list_del_init(prelude_list_t *item)
{
        item->prev->next = item->next;
        item->next->prev = item->prev;
        prelude_list_init(item);
}


/**
 * prelude_list_entry:
 * @item: Pointer to a #prelude_list_t object to retrieve the entry from.
 * @type: Type of the entry to retrieve.
 * @member: List member in @type used to link it to a list.
 *
 * Retrieve the entry of type @type from the #prelude_list_t object @tmp,
 * using the item list member @member. Returns the entry associated with @item.
 */
#define prelude_list_entry(item, type, member)                             \
        (type *) ((unsigned long) item - (unsigned long) &((type *) 0)->member)



/**
 * prelude_list_for_each:
 * @list: Pointer to a #prelude_list_t list.
 * @pos: Pointer to a #prelude_list_t object pointing to the current list member.
 *
 * Iterate through all @list entry. prelude_list_entry() can be used to retrieve
 * and entry from the @pos pointer. It is not safe to call prelude_list_del() while
 * iterating using this function, see prelude_list_for_each_safe().
 */
#define prelude_list_for_each(list, pos)                                   \
        for ( (pos) = (list)->next; (pos) != (list); (pos) = (pos)->next )


/**
 * prelude_list_for_each_safe:
 * @list: Pointer to a #prelude_list_t list.
 * @pos: Pointer to a #prelude_list_t object pointing to the current list member.
 * @bkp: Pointer to a #prelude_list_t object pointing to the next list member.
 *
 * Iterate through all @list entry. prelude_list_entry() can be used to retrieve
 * and entry from the @pos pointer. Calling prelude_list_del() while iterating the
 * list is safe.
 */
#define prelude_list_for_each_safe(list, pos, bkp)                         \
        for ( (pos) = (list)->next, (bkp) = (pos)->next; (pos) != (list); (pos) = (bkp), (bkp) = (pos)->next )



/**
 * prelude_list_for_each_reversed:
 * @list: Pointer to a #prelude_list_t list.
 * @pos: Pointer to a #prelude_list_t object pointing to the current list member.
 *
 * Iterate through all @list entry in reverse order. prelude_list_entry() can be
 * used to retrieve and entry from the @pos pointer. It is not safe to call
 * prelude_list_del() while iterating using this function, see
 * prelude_list_for_each_reversed_safe().
 */
#define prelude_list_for_each_reversed(list, pos)                          \
        for ( (pos) = (list)->prev; (pos) != (list); (pos) = (pos)->prev ) 



/**
 * prelude_list_for_each_reversed_safe:
 * @list: Pointer to a #prelude_list_t list.
 * @pos: Pointer to a #prelude_list_t object pointing to the current list member.
 * @bkp: Pointer to a #prelude_list_t object pointing to the next list member.
 *
 * Iterate through all @list entry in reverse order. prelude_list_entry() can be used to retrieve
 * and entry from the @pos pointer. Calling prelude_list_del() while iterating the
 * list is safe.
 */
#define prelude_list_for_each_reversed_safe(list, pos, bkp)                \
        for ( (pos) = (list)->prev, (bkp) = (pos)->prev; (pos) != (list); (pos) = (bkp), (bkp) = (pos)->prev )


/**
 * prelude_list_for_each_continue:
 * @list: Pointer to a #prelude_list_t list.
 * @pos: Pointer to a #prelude_list_t object pointing to the current list member.
 *
 * Iterate through all @list entry starting from @pos if it is not NULL, or from
 * the start of @list if it is. prelude_list_entry() can be used to retrieve
 * and entry from the @pos pointer. Calling prelude_list_del() while iterating the
 * list is not safe.
 */
#define prelude_list_for_each_continue(list, pos)                          \
        for ( (pos) = ((pos) == NULL) ? (list)->next : (pos)->next; (pos) != (list); (pos) = (pos)->next )


/**
 * prelude_list_for_each_continue_safe:
 * @list: Pointer to a #prelude_list_t list.
 * @pos: Pointer to a #prelude_list_t object pointing to the current list member.
 * @bkp: Pointer to a #prelude_list_t object pointing to the next list member.
 * 
 * Iterate through all @list entry starting from @pos if it is not NULL, or from
 * the start of @list if it is. prelude_list_entry() can be used to retrieve
 * and entry from the @pos pointer. Calling prelude_list_del() while iterating the
 * list is safe.
 */
#define prelude_list_for_each_continue_safe(list, pos, bkp)                \
        for ( (pos) = ((bkp) == NULL) ? (list)->next : (bkp); (bkp) = (pos)->next, (pos) != (list); (pos) = (bkp) )



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


#ifdef __cplusplus
  }
#endif

#endif
