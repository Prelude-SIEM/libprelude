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

#ifndef _LIBPRELUDE_PRELUDE_CLIENT_MGR_H
#define _LIBPRELUDE_PRELUDE_CLIENT_MGR_H

typedef struct prelude_client_mgr prelude_client_mgr_t;


void prelude_client_mgr_broadcast(prelude_client_mgr_t *cmgr, prelude_msg_t *msg);

void prelude_client_mgr_broadcast_async(prelude_client_mgr_t *cmgr, prelude_msg_t *msg);

prelude_client_mgr_t *prelude_client_mgr_new(int type, const char *cfgline);

void prelude_client_mgr_notify_connection(prelude_client_mgr_t *mgr, void (*callback)(prelude_list_t *clist));

prelude_list_t *prelude_client_mgr_get_client_list(prelude_client_mgr_t *mgr);

int prelude_client_mgr_add_client(prelude_client_mgr_t **mgr_ptr, prelude_client_t *client, int use_timer);

int prelude_client_mgr_set_client_dead(prelude_client_mgr_t *mgr, prelude_client_t *client);

prelude_client_t *prelude_client_mgr_search_client(prelude_client_mgr_t *mgr, const char *addr, int type);

int prelude_client_mgr_flush_backup(prelude_client_mgr_t *mgr, prelude_client_t *client);

int prelude_client_mgr_tell_client_dead(prelude_client_mgr_t *mgr, prelude_client_t *client);
int prelude_client_mgr_tell_client_alive(prelude_client_mgr_t *mgr, prelude_client_t *client);


void prelude_client_mgr_destroy(prelude_client_mgr_t *mgr);

#endif /* _LIBPRELUDE_PRELUDE_CLIENT_MGR_H */
