/*****
*
* Copyright (C) 2001 Jeremie Brebec / Toussaint Mathieu
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

#ifndef SSL_CONFIG_H

#define SSL_CONFIG_H


/*
 * Section of ssl-plugin configuratio
 */
#define SSL_SECTION "ssl"

/*
 * Length of RSA-private key
 */
#define KEY_LENGTH 1024

/*
 * Number of days to make a certificate valid for.
 */
#define CERT_DAYS 3650

/*
 * Directory where to search for certificates
 */
#define CERT_DIRECTORY CONFIG_DIR

/*
 * Port of registration for report
 */
#define REGISTRATION_PORT 5554


/*
 * Prelude private key.
 */
#define PRELUDE_KEY "prelude.key"

/*
 * File containing report server certificate
 * (this is on the Prelude side).
 */
#define REPORT_CERT "prelude-report-server.certs"

/*
 * Prelude Report private key.
 */
#define REPORT_KEY "prelude-report.key"

/*
 * File containing Prelude Client certificate
 * (this is on the Prelude Report side).
 */
#define PRELUDE_CERTS "prelude.certs"


void ssl_read_config(config_t *cfg);

int ssl_get_key_length(void);

int ssl_get_days(void);

const char *ssl_get_cert_filename(const char *filename);

char *ssl_get_server_addr(void);

int ssl_get_server_port(void);

#endif
