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

#ifndef _LIBPRELUDE_PRELUDE_CONNECTION_MGR_H
#define _LIBPRELUDE_PRELUDE_CONNECTION_MGR_H

#include "prelude-list.h"
#include "prelude-connection.h"


typedef struct prelude_connection_mgr prelude_connection_mgr_t;


void prelude_connection_mgr_broadcast(prelude_connection_mgr_t *cmgr, prelude_msg_t *msg);

void prelude_connection_mgr_broadcast_async(prelude_connection_mgr_t *cmgr, prelude_msg_t *msg);

prelude_connection_mgr_t *prelude_connection_mgr_new(prelude_client_t *client, const char *cfgline);

void prelude_connection_mgr_notify_connection(prelude_connection_mgr_t *mgr, void (*callback)(prelude_list_t *clist));

prelude_list_t *prelude_connection_mgr_get_connection_list(prelude_connection_mgr_t *mgr);

int prelude_connection_mgr_add_connection(prelude_connection_mgr_t **mgr_ptr, prelude_connection_t *cnx, int use_timer);

int prelude_connection_mgr_set_connection_dead(prelude_connection_mgr_t *mgr, prelude_connection_t *cnx);

prelude_connection_t *prelude_connection_mgr_search_connection(prelude_connection_mgr_t *mgr, const char *addr);

int prelude_connection_mgr_flush_backup(prelude_connection_mgr_t *mgr, prelude_connection_t *cnx);

int prelude_connection_mgr_tell_connection_dead(prelude_connection_mgr_t *mgr, prelude_connection_t *cnx);
int prelude_connection_mgr_tell_connection_alive(prelude_connection_mgr_t *mgr, prelude_connection_t *cnx);

void prelude_connection_mgr_destroy(prelude_connection_mgr_t *mgr);

#endif /* _LIBPRELUDE_PRELUDE_CONNECTION_MGR_H */
