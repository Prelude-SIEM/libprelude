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

#ifndef _LIBPRELUDE_PRELUDE_CONNECTION_POOL_H
#define _LIBPRELUDE_PRELUDE_CONNECTION_POOL_H

#include "prelude-list.h"
#include "prelude-connection.h"


typedef enum {
        PRELUDE_CONNECTION_POOL_FLAGS_RECONNECT        = 0x01,
} prelude_connection_pool_flags_t;


typedef enum {
        PRELUDE_CONNECTION_POOL_EVENT_INPUT            = 0x01,
        PRELUDE_CONNECTION_POOL_EVENT_DEAD             = 0x02,
        PRELUDE_CONNECTION_POOL_EVENT_ALIVE            = 0x04
} prelude_connection_pool_event_t;

typedef struct prelude_connection_pool prelude_connection_pool_t;


void prelude_connection_pool_broadcast(prelude_connection_pool_t *pool, prelude_msg_t *msg);

void prelude_connection_pool_broadcast_async(prelude_connection_pool_t *pool, prelude_msg_t *msg);

int prelude_connection_pool_init(prelude_connection_pool_t *new);

int prelude_connection_pool_new(prelude_connection_pool_t **ret,
                                prelude_client_profile_t *cp,
                                prelude_connection_capability_t capability);

prelude_list_t *prelude_connection_pool_get_connection_list(prelude_connection_pool_t *pool);

int prelude_connection_pool_add_connection(prelude_connection_pool_t *pool, prelude_connection_t *cnx);

int prelude_connection_pool_set_connection_dead(prelude_connection_pool_t *pool, prelude_connection_t *cnx);
int prelude_connection_pool_set_connection_alive(prelude_connection_pool_t *pool, prelude_connection_t *cnx);

int prelude_connection_pool_set_connection_string(prelude_connection_pool_t *pool, const char *cfgstr);

const char *prelude_connection_pool_get_connection_string(prelude_connection_pool_t *pool);

void prelude_connection_pool_destroy(prelude_connection_pool_t *pool);

void prelude_connection_pool_set_flags(prelude_connection_pool_t *pool, prelude_connection_pool_flags_t flags);

void prelude_connection_pool_set_data(prelude_connection_pool_t *pool, void *data);

void *prelude_connection_pool_get_data(prelude_connection_pool_t *pool);

int prelude_connection_pool_recv(prelude_connection_pool_t *pool, prelude_msg_t **out, int timeout);

void prelude_connection_pool_set_global_event_handler(prelude_connection_pool_t *pool,
                                                      prelude_connection_pool_event_t events,
                                                      int (*callback)(prelude_connection_pool_t *pool,
                                                                      prelude_connection_pool_event_t events));

void prelude_connection_pool_set_event_handler(prelude_connection_pool_t *pool,
                                               prelude_connection_pool_event_t events,
                                               int (*callback)(prelude_connection_pool_t *pool,
                                                               prelude_connection_pool_event_t events,
                                                               prelude_connection_t *cnx));

#endif /* _LIBPRELUDE_PRELUDE_CONNECTION_POOL_H */