/*****
*
* Copyright (C) 2001 Yoann Vandoorselaere <yoann@mandrakesoft.com>
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
#include <assert.h>
#include <sys/uio.h>
#include <pthread.h>
#include <inttypes.h>
#include <netinet/in.h>

#include "list.h"
#include "common.h"
#include "alert-read.h"


#define PROTOCOL_VERSION 0

struct alert_container {
        struct list_head list;

        uint32_t msg_index;
        uint32_t max_msgcount;
        unsigned char msg[0];      
};




alert_container_t *prelude_alert_read(int fd, uint8_t *tag) 
{
        int ret;
        uint32_t dlen;
        unsigned char buf[2];
        alert_container_t *ac;

        ret = read(fd, buf, sizeof(buf));
        if ( ret <= 0 )
                return NULL;

        if ( buf[0] != PROTOCOL_VERSION ) {
                log(LOG_ERR, "protocol used isn't the same : (version %d).\n", buf[0]);
                return NULL;
        }

        *tag = buf[1];
        
        ret = read(fd, &dlen, sizeof(dlen));
        if ( ret != sizeof(dlen) ) {
                log(LOG_ERR, "Invalid alert. Couldn't read len field.\n");
                return NULL;
        }
        
        dlen = ntohl(dlen);
        
        ac = malloc(sizeof(alert_container_t) + dlen);
        if ( ! ac ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        ac->max_msgcount = dlen;
        
        ret = read(fd, ac->msg, dlen);
        if ( ret != dlen ) {
                free(ac);
                return NULL;
        }
                
        ac->msg_index = 0;
        
        return ac;
}




int prelude_alert_read_msg(alert_container_t *ac, uint8_t *tag, uint32_t *len, unsigned char **buf) 
{
        if ( ac->msg_index == ac->max_msgcount ) 
                return 0;
        
        if ( (ac->msg_index + 6) > ac->max_msgcount ) 
                return -1;
        
        *tag = ac->msg[ac->msg_index++];        
        *len = ntohl( (*(uint32_t *)&ac->msg[ac->msg_index]) );
        
        ac->msg_index += 4;
        
        if ( (ac->msg_index + *len + 1) > ac->max_msgcount ) 
                return -1;
                
        *buf = &ac->msg[ac->msg_index];
        ac->msg_index += *len;

        /*
         * Skip end of message.
         */
        ac->msg_index++;
        
        return 1;
}













