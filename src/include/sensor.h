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

void prelude_sensor_send_alert(prelude_msg_t *msg);

prelude_msg_t *prelude_sensor_get_option_msg(void);

int prelude_sensor_init(const char *sname, const char *filename, int argc, char **argv);

uint64_t prelude_sensor_get_ident(void);

void prelude_sensor_set_ident(uint64_t *ident);

void prelude_set_sensor_name(const char *sname);

struct list_head *prelude_sensor_get_client_list(void);

void prelude_sensor_notify_mgr_connection(void (*cb)(struct list_head *clist));
