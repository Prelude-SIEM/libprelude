/*****
*
* Copyright (C) 2004 Yoann Vandoorselaere <yoann@prelude-ids.org>
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

int tls_request_certificate(prelude_client_t *client, prelude_io_t *fd, gnutls_x509_privkey key);

int tls_handle_certificate_request(prelude_client_t *client, prelude_io_t *fd,
                                   gnutls_x509_privkey cakey, gnutls_x509_crt cacrt,
                                   gnutls_x509_crt crt);

gnutls_x509_privkey tls_load_privkey(prelude_client_t *client);


int tls_load_ca_certificate(prelude_client_t *client,
                            gnutls_x509_privkey key, gnutls_x509_crt *crt);

int tls_load_ca_signed_certificate(prelude_client_t *client,
                                   gnutls_x509_privkey cakey,
                                   gnutls_x509_crt cacrt,
                                   gnutls_x509_crt *crt);
