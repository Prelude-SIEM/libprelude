/*****
*
* Copyright (C) 2002 Yoann Vandoorselaere <yoann@prelude-ids.org>
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "common.h"
#include "prelude-log.h"
#include "extract.h"
#include "prelude-io.h"
#include "prelude-client.h"
#include "prelude-message.h"
#include "prelude-message-id.h"

#include "client-ident.h"


static void file_error(prelude_client_t *client) 
{
        log(LOG_INFO, "\nBasic file configuration does not exist. Please run :\n"
            "sensor-adduser --sensorname %s --uid %d --gid %d\n"
            "program on the sensor host to create an account for this sensor.\n\n"
            
            "Be aware that you should also pass the \"--manager-addr\" option with the\n"
            "manager address as argument. \"sensor-adduser\" should be called for\n"
            "each configured manager address.\n\n", 
            prelude_client_get_name(client),
            prelude_client_get_uid(client), prelude_client_get_gid(client));

        exit(1);
}



static int declare_ident_to_manager(uint64_t analyzerid, prelude_io_t *fd) 
{
        int ret;
        uint64_t nident;
        prelude_msg_t *msg;
        
        msg = prelude_msg_new(1, sizeof(uint64_t), PRELUDE_MSG_ID, 0);
        if ( ! msg )
                return -1;

        nident = prelude_hton64(analyzerid);
        
        /*
         * send message
         */
        prelude_msg_set(msg, PRELUDE_MSG_ID_DECLARE, sizeof(uint64_t), &nident);
        ret = prelude_msg_write(msg, fd);
        prelude_msg_destroy(msg);

        return ret;
}




int prelude_client_ident_send(uint64_t analyzerid, prelude_io_t *fd) 
{
        return declare_ident_to_manager(analyzerid, fd);
}




int prelude_client_ident_init(prelude_client_t *client, uint64_t *analyzerid) 
{
        int ret;
        FILE *fd;
        char buf[1024], *name, *ident, *ptr;
        
        fd = fopen(PRELUDE_IDENT_FILE, "r");
        if ( ! fd ) {
                log(LOG_ERR, "error opening sensors identity file: %s.\n", PRELUDE_IDENT_FILE);
                file_error(client);
                return -1;
        }

        ptr = buf;
        while ( fgets(buf, sizeof(buf), fd) ) {
                
                name = strtok(ptr, ":");
                if ( ! name )
                        break;
                
                ident = strtok(NULL, ":");
                if ( ! ident )
                        break;

                sscanf(ident, "%llu", analyzerid);
                            
                ret = strcmp(name, prelude_client_get_name(client));
                if ( ret == 0 ) {
                        fclose(fd);
                        return 0;
                }
        }
                
        log(LOG_INFO, "No ident configured for sensor %s.\n", prelude_client_get_name(client));

        file_error(client);
        fclose(fd);
        
        return -1;
}
