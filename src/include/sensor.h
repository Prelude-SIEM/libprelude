/*****
*
* Copyright (C) 2001, 2002 Yoann Vandoorselaere <yoann@prelude-ids.org>
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

#ifndef _LIBPRELUDE_SENSOR_H
#define _LIBPRELUDE_SENSOR_H

#include "idmef.h"
#include "prelude-message.h"

void prelude_sensor_send_msg(prelude_msg_t *msg);

void prelude_sensor_send_msg_async(prelude_msg_t *msg);

int prelude_sensor_init(const char *sname, const char *filename, int argc, char **argv);

prelude_list_t *prelude_sensor_get_client_list(void);

void prelude_sensor_notify_mgr_connection(void (*cb)(prelude_list_t *clist));

void prelude_heartbeat_register_cb(void (*cb)(void *data), void *data);

int prelude_heartbeat_send(void);

int prelude_analyzer_fill_infos(idmef_analyzer_t *analyzer);

#endif /* _LIBPRELUDE_SENSOR_H */
