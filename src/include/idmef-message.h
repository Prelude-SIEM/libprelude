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

#ifndef _LIBPRELUDE_IDMEF_MESSAGE_H
#define _LIBPRELUDE_IDMEF_MESSAGE_H

#include <stdio.h>

#include "prelude-io.h"
#include "prelude-message.h"

idmef_message_t *idmef_message_new(void);
void idmef_message_destroy(idmef_message_t *message);

void idmef_message_set_pmsg(idmef_message_t *message, prelude_msg_t *msg);

int idmef_message_set(idmef_message_t *message, idmef_object_t *object, idmef_value_t *value);

idmef_value_t *idmef_message_get(idmef_message_t *message, idmef_object_t *object);

#endif /* _LIBPRELUDE_IDMEF_MESSAGE_H */
