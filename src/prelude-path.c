/*****
*
* Copyright (C) 2002 Yoann Vandoorselaere <yoann@mandrakesoft.com>
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
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#include "prelude-path.h"


/*
 * directory where backup are made (in case Manager are down).
 */
#define BACKUP_DIR "/var/spool/prelude-sensors"

/*
 * directory where plaintext authentication file are stored.
 */
#define AUTH_DIR "/var/lib/prelude-sensors/plaintext"

/*
 * directory where SSL authentication file are stored.
 */
#define SSL_DIR "/var/lib/prelude-sensors/ssl"

/*
 * Path to the Prelude Unix socket.
 */
#define UNIX_SOCKET "/tmp/.prelude-unix"




static uid_t userid = 0;
static const char *sensorname = NULL;



void prelude_get_auth_filename(char *buf, size_t size) 
{
        snprintf(buf, size, "%s/%s.%d", AUTH_DIR, sensorname, userid);
}


void prelude_get_ssl_cert_filename(char *buf, size_t size) 
{
        snprintf(buf, size, "%s/%s-cert.%d", SSL_DIR, sensorname, userid);
}


void prelude_get_ssl_key_filename(char *buf, size_t size) 
{
        snprintf(buf, size, "%s/%s-key.%d", SSL_DIR, sensorname, userid);
}


void prelude_get_backup_filename(char *buf, size_t size) 
{
        snprintf(buf, size, BACKUP_DIR"/backup.%d", userid);
}


const char *prelude_get_socket_filename(void) 
{
        return UNIX_SOCKET;
}



void prelude_set_program_userid(uid_t uid) 
{
        userid = uid;
}


uid_t prelude_get_program_userid(void) 
{
        return userid;
}



void prelude_set_program_name(const char *sname) 
{
        if ( ! userid )
                userid = getuid();
        
        sensorname = sname;
}



const char *prelude_get_sensor_name(void)
{
        return sensorname;
}
