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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>

#include "prelude-inttypes.h"
#include "prelude-error.h"

#include "common.h"
#include "prelude-log.h"
#include "prelude-client.h"
#include "prelude-message-id.h"

#include "client-ident.h"


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
        FILE *fd;
        char filename[256], buf[256];

        prelude_client_get_ident_filename(client, filename, sizeof(filename));
        
        fd = fopen(filename, "r");
        if ( ! fd )
                return prelude_error_make(PRELUDE_ERROR_SOURCE_CLIENT, PRELUDE_ERROR_ANALYZERID_FILE);

        if ( ! fgets(buf, sizeof(buf), fd) ) {
                fclose(fd);
                return prelude_error_make(PRELUDE_ERROR_SOURCE_CLIENT, PRELUDE_ERROR_ANALYZERID_PARSE);
        }
        
        sscanf(buf, "%" PRIu64, analyzerid);

        fclose(fd);

        return 0;
}
