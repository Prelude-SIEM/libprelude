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
#include <unistd.h>
#include <inttypes.h>

#include "prelude-io.h"
#include "prelude-message.h"
#include "prelude-client.h"
#include "prelude-client-mgr.h"

static prelude_client_t *client;


int prelude_client_mgr_init(const char *addr, uint16_t port) 
{
        client = prelude_client_new(addr, port);
        if ( ! client )
                return -1;

        return 0;
}



int prelude_client_mgr_broadcast_msg(prelude_msg_t *msg) 
{
        return prelude_client_send_msg(client, msg);
}




#if 0
static LIST_HEAD(manager_list);
static LIST_HEAD(backup_manager_list);



static int create_manager_list(config_t *cfg, const char *entry, struct list_head *head) 
{
        const char *addr, *port;
        prelude_client_t *client;
        
        while ( (addr = config_get(cfg, NULL, entry)) ) {

                port = strchr(addr, ':');
                if ( ! port )
                        port = "5554";
                else
                        port++;
                
                client = prelude_client_new();
                if ( ! client )
                        return -1;

                prelude_client_set_manager(strdup(addr), atoi(port));

                prelude_list_add_tail((prelude_linked_object_t *) &client, head);
        }

        return 0;
}




int prelude_client_mgr_broadcast_message(prelude_msg_t *msg) 
{
        struct list_head *tmp;
        prelude_client_t *client;
        
        list_for_each(tmp, &manager_list) {

                client = prelude_list_get_object(tmp, prelude_client_t);

                prelude_client_send_msg(client, msg);
        }
}





int prelude_client_mgr_init(config_t *cfg) 
{
        int ret;

        ret = create_manager_list(cfg, "manager", &manager_list);
        if ( ret < 0 || list_empty(&manager_list) )
                return -1;

        ret = create_manager_list(cfg, "backup manager", &backup_manager_list);
        if ( ret < 0 )
                return -1;

        return 0;
}
#endif
