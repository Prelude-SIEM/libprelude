/*****
*
* Copyright (C) 2001, 2002 Yoann Vandoorselaere <yoann@mandrakesoft.com>
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

typedef void (*prelude_async_func_t)(void *object, void *data);



#define PRELUDE_ASYNC_OBJECT                   \
        PRELUDE_LINKED_OBJECT;                 \
        void *data;                            \
        prelude_async_func_t func


typedef struct {
        PRELUDE_ASYNC_OBJECT;
} prelude_async_object_t;



static inline void prelude_async_set_data(prelude_async_object_t *obj, void *data) 
{
        obj->data = data;
}


static inline void prelude_async_set_callback(prelude_async_object_t *obj, prelude_async_func_t func) 
{
        obj->func = func;
}

int prelude_async_init(void);

void prelude_async_add(prelude_async_object_t *obj);

void prelude_async_del(prelude_async_object_t *obj);










