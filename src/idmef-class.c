/*****
*
* Copyright (C) 2002-2005,2006,2007 PreludeIDS Technologies. All Rights Reserved.
* Author: Yoann Vandoorselaere <yoann.v@prelude-ids.com>
* Author: Krzysztof Zaraska
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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdarg.h>

#include "libmissing.h"
#include "prelude-list.h"
#include "prelude-log.h"
#include "prelude-inttypes.h"
#include "prelude-string.h"

#define PRELUDE_ERROR_SOURCE_DEFAULT PRELUDE_ERROR_SOURCE_IDMEF_TYPE
#include "prelude-error.h"

#include "idmef-time.h"
#include "idmef-data.h"
#include "idmef-value.h"
#include "idmef-class.h"

#include "idmef-tree-wrap.h"
#include "idmef-tree-data.h"
#include "idmef-path.h"



static inline int is_class_valid(idmef_class_id_t class)
{
        if ( class < 0 || (size_t) class >= sizeof(object_data) / sizeof(*object_data) )
                return prelude_error_verbose(PRELUDE_ERROR_IDMEF_CLASS_UNKNOWN, "Unknown IDMEF class '%d'", (int) class);

        return 0;
}


static inline int is_child_valid(idmef_class_id_t class, idmef_class_child_id_t child)
{
        int ret;

        ret = is_class_valid(class);
        if ( ret < 0 )
                return ret;

        if ( child < 0 || (size_t) child >= object_data[class].children_list_elem )
                return prelude_error_verbose(PRELUDE_ERROR_IDMEF_CLASS_UNKNOWN_CHILD, "Unknown IDMEF child '%d' for class '%s'",
                                             (int) child, object_data[class].name);

        return 0;
}




idmef_class_child_id_t idmef_class_find_child(idmef_class_id_t class, const char *name)
{
        int ret;
        size_t i;
        const children_list_t *list;

        ret = is_class_valid(class);
        if ( ret < 0 )
                return ret;

        list = object_data[class].children_list;

        if ( list ) {
                for ( i = 0; i < object_data[class].children_list_elem; i++ )
                        if ( strcasecmp(list[i].name, name) == 0)
                                return i;
        }

        return prelude_error_verbose(PRELUDE_ERROR_IDMEF_CLASS_UNKNOWN_CHILD, "Unknown IDMEF child '%s'", name);
}




prelude_bool_t idmef_class_is_child_list(idmef_class_id_t class, idmef_class_child_id_t child)
{
        int ret;

        ret = is_child_valid(class, child);
        if ( ret < 0 )
                return ret;

        return object_data[class].children_list[child].list;
}




idmef_value_type_id_t idmef_class_get_child_value_type(idmef_class_id_t class, idmef_class_child_id_t child)
{
        int ret;

        ret = is_child_valid(class, child);
        if ( ret < 0 )
                return ret;

        return object_data[class].children_list[child].type;
}




idmef_class_id_t idmef_class_get_child_class(idmef_class_id_t class, idmef_class_child_id_t child)
{
        int ret;
        const children_list_t *c;

        ret = is_child_valid(class, child);
        if ( ret < 0 )
                return ret;

        c = &object_data[class].children_list[child];
        if ( c->type != IDMEF_VALUE_TYPE_CLASS && c->type != IDMEF_VALUE_TYPE_ENUM )
                return prelude_error(PRELUDE_ERROR_IDMEF_CLASS_CHILD_NOT_CLASS);

        return c->class;
}



const char *idmef_class_get_child_name(idmef_class_id_t class, idmef_class_child_id_t child)
{
        int ret;

        ret = is_child_valid(class, child);
        if ( ret < 0 )
                return NULL;

        return object_data[class].children_list[child].name;
}




idmef_class_id_t idmef_class_find(const char *name)
{
        idmef_class_id_t i;

        for ( i = 0; object_data[i].name != NULL; i++ )
                if ( strcasecmp(object_data[i].name, name) == 0 )
                        return i;

        return prelude_error_verbose(PRELUDE_ERROR_IDMEF_CLASS_UNKNOWN_NAME, "Unknown IDMEF class '%s'", name);
}


int idmef_class_enum_to_numeric(idmef_class_id_t class, const char *val)
{
        int ret;

        ret = is_class_valid(class);
        if ( ret < 0 )
                return ret;

        if ( ! object_data[class].to_numeric )
                return -1;

            return object_data[class].to_numeric(val);
}


const char *idmef_class_enum_to_string(idmef_class_id_t class, int val)
{
        int ret;

        ret = is_class_valid(class);
        if ( ret < 0 )
                return NULL;

        if ( ! object_data[class].to_string )
                return NULL;

        return object_data[class].to_string(val);
}


int idmef_class_get_child(void *ptr, idmef_class_id_t class, idmef_class_child_id_t child, void **childptr)
{
        int ret;

        ret = is_child_valid(class, child);
        if ( ret < 0 )
                return ret;

        return object_data[class].get_child(ptr, child, childptr);
}




int idmef_class_new_child(void *ptr, idmef_class_id_t class, idmef_class_child_id_t child, int n, void **childptr)
{
        int ret;

        ret = is_child_valid(class, child);
        if ( ret < 0 )
                return ret;

        return object_data[class].new_child(ptr, child, n, childptr);
}



int idmef_class_destroy_child(void *ptr, idmef_class_id_t class, idmef_class_child_id_t child, int n)
{
        int ret;

        ret = is_child_valid(class, child);
        if ( ret < 0 )
                return ret;

        return object_data[class].destroy_child(ptr, child, n);
}


int idmef_class_copy(idmef_class_id_t class, const void *src, void *dst)
{
        int ret;

        ret = is_class_valid(class);
        if ( ret < 0 )
                return ret;

        return object_data[class].copy(src, dst);
}



int idmef_class_clone(idmef_class_id_t class, const void *src, void **dst)
{
        int ret;

        ret = is_class_valid(class);
        if ( ret < 0 )
                return ret;

        return object_data[class].clone(src, dst);
}



int idmef_class_compare(idmef_class_id_t class, const void *c1, const void *c2)
{
        int ret;

        ret = is_class_valid(class);
        if ( ret < 0 )
                return ret;

        return object_data[class].compare(c1, c2);
}



int idmef_class_destroy(idmef_class_id_t class, void *obj)
{
        int ret;

        ret = is_class_valid(class);
        if ( ret < 0 )
                return ret;

        object_data[class].destroy(obj);

        return 0;
}



int idmef_class_ref(idmef_class_id_t class, void *obj)
{
        int ret;

        ret = is_class_valid(class);
        if ( ret < 0 )
                return ret;

        object_data[class].ref(obj);

        return 0;
}


const char *idmef_class_get_name(idmef_class_id_t class)
{
        if ( is_class_valid(class) < 0 )
                return NULL;

        return object_data[class].name;
}
