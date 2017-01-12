/*****
*
* Copyright (C) 2004-2017 CS-SI. All Rights Reserved.
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
* You should have received a copy of the GNU General Public License along
* with this program; if not, write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*
*****/

#ifndef _LIBPRELUDE_TLS_AUTH_H
#define _LIBPRELUDE_TLS_AUTH_H

#include "prelude-io.h"
#include "prelude-connection.h"


int tls_auth_connection(prelude_client_profile_t *cp, prelude_io_t *io, int crypt,
                        uint64_t *peer_analyzerid, prelude_connection_permission_t *permission);

int tls_auth_init(prelude_client_profile_t *cp, gnutls_certificate_credentials_t *cred);

int tls_auth_init_priority(const char *tlsopts);

void tls_auth_deinit(void);

#endif
