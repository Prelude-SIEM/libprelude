/*****
*
* Copyright (C) 2003 Krzysztof Zaraska <kzaraska@student.uci.agh.edu.pl>
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

#include "idmef-string.h"
#include "idmef-time.h"
#include "idmef-data.h"

#include "idmef-type.h"
#include "idmef-value.h"

#include "idmef-tree-wrap.h"
#include "idmef-object.h"
#include "idmef-object-value.h"


struct idmef_object_value {
	prelude_list_t list;
	idmef_object_t *object;
	idmef_value_t *value;
};

struct idmef_object_value_list {
	idmef_object_value_t *iterator;
	prelude_list_t value_list;
};




idmef_object_value_t *idmef_object_value_new(idmef_object_t *object, idmef_value_t *value)
{
	idmef_object_value_t *ret;

	if ( ! object || ! value )
		return NULL;

	ret = calloc(1, sizeof(*ret));
	if ( ! ret ) {
		log(LOG_ERR, "out of memory\n");
		return NULL;
	}

	ret->object = object;
	ret->value = value;

	return ret;
}


idmef_object_t *idmef_object_value_get_object(idmef_object_value_t *objval)
{
	return objval ? objval->object : NULL;
}




idmef_value_t *idmef_object_value_get_value(idmef_object_value_t *objval)
{
	return objval ? objval->value : NULL;
}




void idmef_object_value_destroy(idmef_object_value_t *objval)
{
	if ( ! objval )
		return ;
		
	idmef_object_destroy(objval->object);
	idmef_value_destroy(objval->value);

	free(objval);
}




idmef_object_value_list_t *idmef_object_value_list_new(void)
{
	idmef_object_value_list_t *ret;
	
	ret = calloc(1, sizeof(*ret));
	if ( ! ret ) {
		log(LOG_ERR, "out of memory\n");
		return NULL;
	}
	
	PRELUDE_INIT_LIST_HEAD(&ret->value_list);
	
	return ret;
}




int idmef_object_value_list_add(idmef_object_value_list_t *list, idmef_object_value_t *objval)
{
	if ( ! list || ! objval )
		return -1;
	
	prelude_list_add_tail(&objval->list, &list->value_list);
	
	return 0;
}




idmef_object_value_t *idmef_object_value_list_get_next(idmef_object_value_list_t *list)
{
	if ( ! list )
		return NULL;

	list->iterator = prelude_list_get_next(list->iterator, 
                                               &list->value_list,
                                               idmef_object_value_t,
                                               list);

	return list->iterator;
}




void idmef_object_value_list_destroy(idmef_object_value_list_t *list)
{
	prelude_list_t *n, *tmp;
	idmef_object_value_t *entry;

	if ( ! list )
		return ;
		
	prelude_list_for_each_safe(tmp, n, &list->value_list) {
		entry = prelude_list_entry(tmp, idmef_object_value_t, list);

                prelude_list_del(&entry->list);
		idmef_object_value_destroy(entry);
	}
	
	free(list);
}
