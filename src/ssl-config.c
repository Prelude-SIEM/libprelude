#include "config.h"
#ifdef HAVE_SSL

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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

#include "config-engine.h"
#include "ssl-config.h"


/*
 * Length of RSA-private key
 */
#define KEY_LENGTH 1024

/*
 * Number of days to make a certificate valid for.
 */
#define CERT_DAYS 3650


/*
 * Port of registration for report
 */
static struct {
	int key_length;
	int days;
} config;




void ssl_read_config(config_t *cfg, const char *section)
{
        const char *param;

	config.key_length = KEY_LENGTH;
	config.days = CERT_DAYS;

        param = config_get(cfg, section, "key");
        if (param)
                config.key_length = atoi(param);

        param = config_get(cfg, section, "days");
        if (param)
                config.days = atoi(param);
}



int ssl_get_key_length(void)
{
	return config.key_length;
}




int ssl_get_days(void)
{
	return config.days;
}

#endif

