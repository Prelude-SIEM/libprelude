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



static struct {
	int key_length;
	int days;
	char *cert_directory;
} config;




void ssl_read_config(config_t *cfg)
{
        const char *param;

	config.key_length = KEY_LENGTH;
	config.days = CERT_DAYS;
	config.cert_directory = CERT_DIRECTORY;

        param = config_get(cfg, SSL_SECTION, "key");
        if (param)
                config.key_length = atoi(param);

        param = config_get(cfg, SSL_SECTION, "days");
        if (param)
                config.days = atoi(param);
        
        param = config_get(cfg, SSL_SECTION, "directory");
        if (param) 
                config.cert_directory = strdup(param);
}



int ssl_get_key_length(void)
{
	return config.key_length;
}




int ssl_get_days(void)
{
	return config.days;
}




const char *ssl_get_cert_filename(const char *filename)
{
	static char buf[1024];

	snprintf(buf, sizeof(buf), "%s/%s", config.cert_directory, filename);

	return buf;
}

#endif

