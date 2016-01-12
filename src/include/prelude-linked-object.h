/*****
*
* Copyright (C) 2001-2016 CS-SI. All Rights Reserved.
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
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*
*****/

#ifndef _LIBPRELUDE_PRELUDE_LINKED_OBJECT_H
#define _LIBPRELUDE_PRELUDE_LINKED_OBJECT_H


#include "prelude-list.h"


#define PRELUDE_LINKED_OBJECT    \
        unsigned int _object_id; \
        prelude_list_t _list


typedef struct {
        PRELUDE_LINKED_OBJECT;
} prelude_linked_object_t;



static inline void prelude_linked_object_del(prelude_linked_object_t *obj) 
{
        prelude_list_del(&obj->_list);
}



static inline void prelude_linked_object_del_init(prelude_linked_object_t *obj) 
{
        prelude_list_del(&obj->_list);
        prelude_list_init(&obj->_list);
}



static inline void prelude_linked_object_add(prelude_list_t *head, prelude_linked_object_t *obj) 
{
        prelude_list_add(head, &obj->_list);
}



static inline void prelude_linked_object_add_tail(prelude_list_t *head, prelude_linked_object_t *obj) 
{
        prelude_list_add_tail(head, &obj->_list);
}


static inline void prelude_linked_object_set_id(prelude_linked_object_t *obj, unsigned int id)
{
        obj->_object_id = id;
}


static inline unsigned int prelude_linked_object_get_id(prelude_linked_object_t *obj)
{
        return obj->_object_id;
}



#define prelude_linked_object_get_object(object)  \
        (void *) prelude_list_entry(object, prelude_linked_object_t, _list)


#endif /* _LIBPRELUDE_PRELUDE_LINKED_OBJECT_H */
