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
#include "idmef-type.h"

#include "idmef-tree-wrap.h"
#include "idmef-tree-data.h"
#include "idmef-path.h"


idmef_child_t idmef_type_find_child(idmef_object_type_t type, const char *name)
{
        idmef_child_t i;
	const children_list_t *list;
                
	list = object_data[type].children_list;
        
	for ( i = 0; list[i].name; i++ )                
		if ( strcasecmp(list[i].name, name) == 0)
			return i;

	return prelude_error(PRELUDE_ERROR_IDMEF_TYPE_UNKNOWN_NAME);
}




int idmef_type_child_is_list(idmef_object_type_t type, idmef_child_t child)
{
	const children_list_t *c;
        
	if ( type < 0 || child < 0 )
		return prelude_error(PRELUDE_ERROR_IDMEF_TYPE_UNKNOWN);
	
	c = &object_data[type].children_list[child];
	
	return c->list;
}




idmef_value_type_id_t idmef_type_get_child_type(idmef_object_type_t type, idmef_child_t child)
{
	const children_list_t *c;
        
	if ( type < 0 || child < 0 )
		return prelude_error(PRELUDE_ERROR_IDMEF_TYPE_UNKNOWN);
	
	c = &object_data[type].children_list[child];
	
	return c->type;
}




idmef_object_type_t idmef_type_get_child_object_type(idmef_object_type_t type, idmef_child_t child)
{
	const children_list_t *c;

	if ( type < 0 || child < 0 )
		return prelude_error(PRELUDE_ERROR_IDMEF_TYPE_UNKNOWN);
	
	c = &object_data[type].children_list[child];
	
	return (c->type == IDMEF_VALUE_TYPE_OBJECT) ? c->object_type : -1;
}



idmef_object_type_t idmef_type_get_child_enum_type(idmef_object_type_t type, idmef_child_t child)
{
	const children_list_t *c;

	if ( type < 0 || child < 0 )
		return prelude_error(PRELUDE_ERROR_IDMEF_TYPE_UNKNOWN);
	
	c = &object_data[type].children_list[child];
	
	return (c->type == IDMEF_VALUE_TYPE_ENUM) ? c->object_type : -1;
}




char *idmef_type_get_child_name(idmef_object_type_t type, idmef_child_t child)
{
	const children_list_t *c;

	if ( type < 0 || child < 0 )
		return NULL;
	
	c = &object_data[type].children_list[child];
	
	return c->name;	
}




idmef_object_type_t idmef_type_find(const char *name)
{
	idmef_object_type_t i;
	
	for (i=0; object_data[i].name; i++)
		if (strcasecmp(object_data[i].name, name) == 0)
			return i;
			
	return -1;
}


int idmef_type_is_enum(idmef_object_type_t type)
{
    	if ( type < 0 )
		return prelude_error(PRELUDE_ERROR_IDMEF_TYPE_UNKNOWN);
	
	return ( object_data[type].to_string ) ? 1 : 0;
}


int idmef_type_enum_to_numeric(idmef_object_type_t type, const char *val)
{
    	return (type < 0) ? prelude_error(PRELUDE_ERROR_IDMEF_TYPE_UNKNOWN) : object_data[type].to_numeric(val);
}


const char *idmef_type_enum_to_string(idmef_object_type_t type, int val)
{
	return (type < 0) ? NULL : object_data[type].to_string(val);
}


int idmef_type_get_child(void *ptr, idmef_object_type_t type, idmef_child_t child, void **childptr)
{
	if ( type < 0 || child < 0 )
		return prelude_error(PRELUDE_ERROR_IDMEF_TYPE_UNKNOWN);
        
	return object_data[type].get_child(ptr, child, childptr);
}




int idmef_type_new_child(void *ptr, idmef_object_type_t type, idmef_child_t child, int n, void **childptr)
{
	if ( type < 0 || child < 0 )
                return prelude_error(PRELUDE_ERROR_IDMEF_TYPE_UNKNOWN);

	return object_data[type].new_child(ptr, child, n, childptr);
}




char *idmef_type_get_name(idmef_object_type_t type)
{
	return (type < 0) ? NULL : object_data[type].name;
}
