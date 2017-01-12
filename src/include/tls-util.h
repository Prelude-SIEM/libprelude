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

#ifndef _LIBPRELUDE_TLS_UTIL_H
#define _LIBPRELUDE_TLS_UTIL_H


void tls_unload_file(gnutls_datum_t *data);

int tls_load_file(const char *filename, gnutls_datum_t *data);

int tls_certificates_load(gnutls_x509_privkey_t key, const char *certfile, gnutls_certificate_credentials_t cred);

int tls_certificate_get_peer_analyzerid(gnutls_session_t session, uint64_t *analyzerid);

int tls_certificate_get_permission(gnutls_session_t session, prelude_connection_permission_t *permission);

int _prelude_tls_crt_list_import(gnutls_x509_crt_t *certs, unsigned int *cmax,
                                 const gnutls_datum_t *data, gnutls_x509_crt_fmt_t format);

#endif
