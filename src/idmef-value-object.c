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
#include <inttypes.h>
#include <sys/types.h>
#include <stdarg.h>

#include "prelude-list.h"
#include "prelude-log.h"

#include "idmef-string.h"
#include "idmef-time.h"
#include "idmef-data.h"

#include "idmef-type.h"
#include "idmef-value.h"

#include "idmef-tree-wrap.h"
#include "idmef-object.h"
#include "idmef-value-object.h"




idmef_value_t *idmef_value_new_for_object(idmef_object_t *object, const char *buf)
{
    	idmef_object_type_t idmef_type;
    	idmef_value_type_id_t value_type;

    	if ( ! object || ! buf )
	    	return NULL;
		
	value_type = idmef_object_get_value_type(object);
	if ( value_type < 0 )
		    return NULL;
	
	if  ( value_type == IDMEF_VALUE_TYPE_ENUM ) {
	    	idmef_type = idmef_object_get_object_type(object);
	    	if ( idmef_type < 0 )
		    	return NULL;
	    
	    	return idmef_value_new_enum(idmef_type, buf);
	} else {
	    	return idmef_value_new_generic(value_type, buf);
	}
	
	/*
	if ( idmef_type_is_enum(idmef_type) )
		return idmef_value_new_enum_generic(idmef_type, buf);
	else {
	    	value_type = idmef_object_get_type(object);
	    	if ( value_type < 0 )
		    	return NULL;
		
		return idmef_value_new_generic(value_type, buf);
	}
	*/
	return NULL; /* not reached */
}

