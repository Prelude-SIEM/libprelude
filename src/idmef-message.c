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


idmef_message_t *idmef_message_new(void)
{
	idmef_message_t *message;

	message = calloc(1, sizeof (*message));

	return message;
}



void idmef_message_destroy(idmef_message_t *message)
{
	idmef_message_destroy_internal(message);

	if ( message->pmsg )
		prelude_msg_destroy(message->pmsg);

	free(message);
}



void idmef_message_set_pmsg(idmef_message_t *message, prelude_msg_t *pmsg)
{
	message->pmsg = pmsg;
}



int idmef_message_set(idmef_message_t *message, idmef_object_t *object, idmef_value_t *value)
{
	return idmef_object_set(message, object, value);
}



idmef_value_t *idmef_message_get(idmef_message_t *message, idmef_object_t *object)
{
	return idmef_object_get(message, object);
}
