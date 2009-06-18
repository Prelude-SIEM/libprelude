/*****
*
* Copyright (C) 2004, 2005 PreludeIDS Technologies. All Rights Reserved.
* Author: Yoann Vandoorselaere <yoann.v@prelude-ids.com>
*
* This file is part of the Prelude library.
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

#ifndef _LIBPRELUDE_PRELUDE_CLIENT_H
#define _LIBPRELUDE_PRELUDE_CLIENT_H

#ifdef __cplusplus
 extern "C" {
#endif

typedef enum {
        PRELUDE_CLIENT_EXIT_STATUS_SUCCESS = 0,
        PRELUDE_CLIENT_EXIT_STATUS_FAILURE = -1
} prelude_client_exit_status_t;


typedef enum {
        PRELUDE_CLIENT_FLAGS_ASYNC_SEND  = 0x01,
        PRELUDE_CLIENT_FLAGS_ASYNC_TIMER = 0x02,
        PRELUDE_CLIENT_FLAGS_HEARTBEAT   = 0x04,
        PRELUDE_CLIENT_FLAGS_CONNECT     = 0x08,
        PRELUDE_CLIENT_FLAGS_AUTOCONFIG  = 0x10
} prelude_client_flags_t;


typedef struct prelude_client prelude_client_t;


#include "prelude-client-profile.h"
#include "prelude-ident.h"
#include "prelude-connection.h"
#include "prelude-connection-pool.h"
#include "idmef.h"


prelude_ident_t *prelude_client_get_unique_ident(prelude_client_t *client);

void prelude_client_set_connection_pool(prelude_client_t *client, prelude_connection_pool_t *pool);

prelude_connection_pool_t *prelude_client_get_connection_pool(prelude_client_t *client);

int prelude_client_start(prelude_client_t *client);

int prelude_client_init(prelude_client_t *client);

int prelude_client_new(prelude_client_t **client, const char *profile);

prelude_client_t *prelude_client_ref(prelude_client_t *client);

idmef_analyzer_t *prelude_client_get_analyzer(prelude_client_t *client);

prelude_client_flags_t prelude_client_get_flags(prelude_client_t *client);

void prelude_client_set_required_permission(prelude_client_t *client, prelude_connection_permission_t permission);

prelude_connection_permission_t prelude_client_get_required_permission(prelude_client_t *client);

void prelude_client_send_msg(prelude_client_t *client, prelude_msg_t *msg);

int prelude_client_recv_msg(prelude_client_t *client, int timeout, prelude_msg_t **msg);

void prelude_client_set_heartbeat_cb(prelude_client_t *client, void (*cb)(prelude_client_t *client, idmef_message_t *hb));

void prelude_client_send_idmef(prelude_client_t *client, idmef_message_t *msg);

int prelude_client_recv_idmef(prelude_client_t *client, int timeout, idmef_message_t **idmef);

void prelude_client_destroy(prelude_client_t *client, prelude_client_exit_status_t status);

int prelude_client_set_flags(prelude_client_t *client, prelude_client_flags_t flags);

int prelude_client_set_config_filename(prelude_client_t *client, const char *filename);

const char *prelude_client_get_config_filename(prelude_client_t *client);

prelude_client_profile_t *prelude_client_get_profile(prelude_client_t *client);

int prelude_client_new_msgbuf(prelude_client_t *client, prelude_msgbuf_t **msgbuf);

int prelude_client_handle_msg_default(prelude_client_t *client, prelude_msg_t *msg, prelude_msgbuf_t *msgbuf);

int _prelude_client_register_options(void);

#ifndef PRELUDE_DISABLE_DEPRECATED
const char *prelude_client_get_setup_error(prelude_client_t *client);

prelude_bool_t prelude_client_is_setup_needed(int error);
#endif

void prelude_client_print_setup_error(prelude_client_t *client);


#ifdef __cplusplus
 }
#endif

#endif
