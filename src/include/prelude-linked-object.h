/*****
*
* Copyright (C) 2001 Yoann Vandoorselaere <yoann@prelude-ids.org>
* All Rights Reserved
*
* This file is part of the Prelude program.
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

#ifndef _LIBPRELUDE_PRELUDE_LINKED_OBJECT_H
#define _LIBPRELUDE_PRELUDE_LINKED_OBJECT_H


#include "prelude-list.h"


#define PRELUDE_LINKED_OBJECT \
        prelude_list_t list


typedef struct {
        PRELUDE_LINKED_OBJECT;
} prelude_linked_object_t;



static inline void prelude_linked_object_del(prelude_linked_object_t *obj) 
{
        prelude_list_del(&obj->list);
}



static inline void prelude_linked_object_add(prelude_list_t *head, prelude_linked_object_t *obj) 
{
        prelude_list_add(head, &obj->list);
}



static inline void prelude_linked_object_add_tail(prelude_list_t *head, prelude_linked_object_t *obj) 
{
        prelude_list_add_tail(head, &obj->list);
}


#define prelude_linked_object_get_object(object)  \
        (void *) prelude_list_entry(object, prelude_linked_object_t, list)


#endif /* _LIBPRELUDE_PRELUDE_LINKED_OBJECT_H */
