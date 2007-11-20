/*****
*
* Copyright (C) 2004,2005 PreludeIDS Technologies. All Rights Reserved.
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

#ifndef _LIBPRELUDE_TLS_UTIL_H
#define _LIBPRELUDE_TLS_UTIL_H


void tls_unload_file(gnutls_datum *data);

int tls_load_file(const char *filename, gnutls_datum *data);

int tls_certificates_load(gnutls_x509_privkey key, const char *certfile, gnutls_certificate_credentials cred);

int tls_certificate_get_peer_analyzerid(gnutls_session session, uint64_t *analyzerid);

int tls_certificate_get_permission(gnutls_session session, prelude_connection_permission_t *permission);

int _prelude_tls_crt_list_import(gnutls_x509_crt *certs, unsigned int *cmax,
                                 const gnutls_datum *data, gnutls_x509_crt_fmt format);

#endif
