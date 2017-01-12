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

#include "tls-util.h"

int tls_request_certificate(prelude_client_profile_t *cp, prelude_io_t *fd, gnutls_x509_privkey_t key,
                            prelude_connection_permission_t permission);

int tls_handle_certificate_request(const char *srcinfo, prelude_client_profile_t *cp, prelude_io_t *fd,
                                   gnutls_x509_privkey_t cakey, gnutls_x509_crt_t cacrt, gnutls_x509_crt_t crt);

gnutls_x509_privkey_t tls_load_privkey(prelude_client_profile_t *cp);


int tls_load_ca_certificate(prelude_client_profile_t *cp,
                            gnutls_x509_privkey_t key, gnutls_x509_crt_t *crt);

int tls_load_ca_signed_certificate(prelude_client_profile_t *cp,
                                   gnutls_x509_privkey_t cakey,
                                   gnutls_x509_crt_t cacrt,
                                   gnutls_x509_crt_t *crt);

int tls_revoke_analyzer(prelude_client_profile_t *cp, gnutls_x509_privkey_t key,
                        gnutls_x509_crt_t crt, uint64_t revoked_analyzerid);
