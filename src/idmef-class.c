/*****
*
* Copyright (C) 2002,2003 Krzysztof Zaraska <kzaraska@student.uci.agh.edu.pl>
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdarg.h>

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


idmef_class_child_id_t idmef_class_find_child(idmef_class_id_t class, const char *name)
{
        idmef_class_child_id_t i;
	const children_list_t *list;
                
	list = object_data[class].children_list;
        
	for ( i = 0; list[i].name; i++ )                
		if ( strcasecmp(list[i].name, name) == 0)
			return i;

	return prelude_error(PRELUDE_ERROR_IDMEF_TYPE_UNKNOWN_NAME);
}




prelude_bool_t idmef_class_is_child_list(idmef_class_id_t class, idmef_class_child_id_t child)
{
	if ( class < 0 || child < 0 )
		return prelude_error(PRELUDE_ERROR_IDMEF_TYPE_UNKNOWN);
	
	return object_data[class].children_list[child].list;
}




idmef_value_type_id_t idmef_class_get_child_value_type(idmef_class_id_t class, idmef_class_child_id_t child)
{        
	if ( class < 0 || child < 0 || ! object_data[class].children_list )
		return prelude_error(PRELUDE_ERROR_IDMEF_TYPE_UNKNOWN);
                
        return object_data[class].children_list[child].type;
}




idmef_class_id_t idmef_class_get_child_class(idmef_class_id_t class, idmef_class_child_id_t child)
{
	const children_list_t *c;

	if ( class < 0 || child < 0 )
		return prelude_error(PRELUDE_ERROR_IDMEF_TYPE_UNKNOWN);
	
	c = &object_data[class].children_list[child];
        
        return (c->type == IDMEF_VALUE_TYPE_CLASS || c->type == IDMEF_VALUE_TYPE_ENUM ) ? c->class : -1;
}



const char *idmef_class_get_child_name(idmef_class_id_t class, idmef_class_child_id_t child)
{
	if ( class < 0 || child < 0 )
		return NULL;
	
	return object_data[class].children_list[child].name;
}




idmef_class_id_t idmef_class_find(const char *name)
{
	idmef_class_id_t i;
	
	for ( i = 0; object_data[i].name != NULL; i++ )
		if ( strcasecmp(object_data[i].name, name) == 0 )
			return i;
        
	return -1;
}


int idmef_class_enum_to_numeric(idmef_class_id_t class, const char *val)
{
    	return (class < 0) ? prelude_error(PRELUDE_ERROR_IDMEF_TYPE_UNKNOWN) : object_data[class].to_numeric(val);
}


const char *idmef_class_enum_to_string(idmef_class_id_t class, int val)
{
	return (class < 0) ? NULL : object_data[class].to_string(val);
}


int idmef_class_get_child(void *ptr, idmef_class_id_t class, idmef_class_child_id_t child, void **childptr)
{
	if ( class < 0 || child < 0 )
		return prelude_error(PRELUDE_ERROR_IDMEF_TYPE_UNKNOWN);
        
	return object_data[class].get_child(ptr, child, childptr);
}




int idmef_class_new_child(void *ptr, idmef_class_id_t class, idmef_class_child_id_t child, int n, void **childptr)
{
	if ( class < 0 || child < 0 )
                return prelude_error(PRELUDE_ERROR_IDMEF_TYPE_UNKNOWN);

	return object_data[class].new_child(ptr, child, n, childptr);
}




const char *idmef_class_get_name(idmef_class_id_t class)
{
	return (class < 0) ? NULL : object_data[class].name;
}
