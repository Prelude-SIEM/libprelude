/*****
*
* Copyright (C) 2003 Nicolas Delon <delon.nicolas@wanadoo.fr>
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

#include "list.h"
#include "prelude-hash.h"
#include "prelude-log.h"

#include "idmef-string.h"
#include "idmef-time.h"
#include "idmef-data.h"

#include "idmef-type.h"
#include "idmef-value.h"

#include "idmef-tree.h"
#include "idmef-tree-wrap.h"
#include "idmef-object.h"
#include "idmef-object-value.h"

#include "idmef-message.h"


void my_idmef_object_value_destroy(void *key)
{
	idmef_object_value_t *val = key;
	
	idmef_object_value_destroy(val);
}


idmef_message_t *idmef_message_new(void)
{
	idmef_message_t *message;

	message = calloc(1, sizeof (*message));

	return message;
}



void idmef_message_destroy(idmef_message_t *message)
{
	idmef_message_destroy_internal(message);

	if ( message->cache )
		prelude_hash_destroy(message->cache);

	if ( message->pmsg )
		prelude_msg_destroy(message->pmsg);

	free(message);
}



void idmef_message_set_pmsg(idmef_message_t *message, prelude_msg_t *pmsg)
{
	message->pmsg = pmsg;
}



int idmef_message_enable_cache(idmef_message_t *message)
{
	if ( ! message )
		return -1;

	message->cache = prelude_hash_new(NULL, NULL, NULL, my_idmef_object_value_destroy);

	return message->cache ? 0 : -1;
}


int idmef_message_disable_cache(idmef_message_t *message)
{
	prelude_hash_destroy(message->cache);
	message->cache = NULL;

	return 0;
}



int idmef_message_set(idmef_message_t *message, idmef_object_t *object, idmef_value_t *value)
{
	if ( idmef_object_set(message, object, value) < 0 )
		return -1;

	if ( message->cache && ! idmef_object_is_ambiguous(object) ) {
		idmef_object_value_t *object_value;

		/*
		 * here, we use idmef_object_ref and idmef_value_ref so that if prelude_hash_set
		 * fail and idmef_object_value_destroy destroy his object and value, the object
		 * and value passed as arguments to the function will still be valid for the caller
		 * 
		 * if everything goes fine, the refcounts will be decremented by idmef_object_destroy
		 * and idmef_value_destroy just before idmef_message_set returns 0
		 */

		object_value = idmef_object_value_new(idmef_object_ref(object), idmef_value_ref(value));
		if ( ! object_value ) {
			idmef_object_destroy(object);
			idmef_value_destroy(value);
			return -1;
		}

		if ( prelude_hash_set(message->cache, idmef_object_get_name(object), object_value) < 0 ) {
			idmef_object_value_destroy(object_value);
			return -1;
		}

	}

	idmef_object_destroy(object);
	idmef_value_destroy(value);

	return 0;
}



idmef_value_t *idmef_message_get(idmef_message_t *message, idmef_object_t *object)
{
	idmef_value_t *value;

	if ( message->cache ) {
		idmef_object_value_t *object_value;

		object_value = prelude_hash_get(message->cache, idmef_object_get_name(object));
		if ( object_value && (value = idmef_object_value_get_value(object_value)) )
			return idmef_value_ref(value);
	}

	value = idmef_object_get(message, object);
	if ( ! value )
		return NULL;

	if ( message->cache ) {
		idmef_object_value_t *object_value;

		object_value = idmef_object_value_new(idmef_object_ref(object), value);
		if ( ! object_value ) {
			idmef_value_destroy(value);
			return NULL;
		}

		if ( prelude_hash_set(message->cache, idmef_object_get_name(object), object_value) < 0 ) {
			idmef_object_value_destroy(object_value);
			return NULL;
		}
		
		return idmef_value_ref(value);
	}

	return value;
}
