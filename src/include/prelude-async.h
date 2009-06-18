/*****
*
* Copyright (C) 2001, 2002, 2003, 2004, 2005 PreludeIDS Technologies. All Rights Reserved.
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

#ifndef _LIBPRELUDE_PRELUDE_ASYNC_H
#define _LIBPRELUDE_PRELUDE_ASYNC_H


#include "prelude-linked-object.h"

#ifdef __cplusplus
 extern "C" {
#endif


/**
 * prelude_async_flags_t
 * @PRELUDE_ASYNC_FLAGS_TIMER: Enable asynchronous timer.
 *
 * This provides asynchronous timer. When enabled, the heartbeat
 * function (and user specified callback, if any) will be called
 * automatically, from an asynchronous thread.
 *
 * If you use this flags, you won't need to call prelude_wake_up_timer()
 * anymore.
 */
typedef enum {
        PRELUDE_ASYNC_FLAGS_TIMER   = 0x01
} prelude_async_flags_t;

typedef void (*prelude_async_callback_t)(void *object, void *data);



#define PRELUDE_ASYNC_OBJECT                   \
        PRELUDE_LINKED_OBJECT;                 \
        void *_async_data;                     \
        prelude_async_callback_t _async_func


typedef struct {
        PRELUDE_ASYNC_OBJECT;
} prelude_async_object_t;



static inline void prelude_async_set_data(prelude_async_object_t *obj, void *data)
{
        obj->_async_data = data;
}


static inline void prelude_async_set_callback(prelude_async_object_t *obj, prelude_async_callback_t func)
{
        obj->_async_func = func;
}

int prelude_async_init(void);

prelude_async_flags_t prelude_async_get_flags(void);

void prelude_async_set_flags(prelude_async_flags_t flags);

void prelude_async_add(prelude_async_object_t *obj);

void prelude_async_del(prelude_async_object_t *obj);

void prelude_async_exit(void);


void _prelude_async_fork_prepare(void);
void _prelude_async_fork_parent(void);
void _prelude_async_fork_child(void);


#ifdef __cplusplus
 }
#endif

#endif /* _LIBPRELUDE_PRELUDE_ASYNC_H */

