/*
 *  Copyright (C) 2000 Yoann Vandoorselaere.
 *
 *  This program is free software; you can redistribute it and/or modify 
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Authors: Yoann Vandoorselaere <yoann@mandrakesoft.com>
 *
 */
#include <stdio.h>
#include <stdlib.h>

#include "list.h"

#include "common.h"
#include "plugin-common.h"
#include "alert.h"
#include "alert-common.h"
#include "socket-op.h"


#define do_read(fd, ptr, size, mread) do {                    \
        if ( socket_read_nowait(fd, ptr, size, mread) <= 0)   \
               goto err;                                      \
} while(0)



/*
 * I would like OpenSSL to support readv()...
 * It would be a lot cleaner, and a lot faster...
 */
static int read_alert(int fd, alert_t *alert,
                      ssize_t (*my_read)(int fd, void *data, size_t size)) 
{
        int ret = 0;
        plugin_generic_t *p = alert_plugin(alert);

        ret = socket_read(fd, &plugin_name_len(p), sizeof(plugin_name_len(p)), my_read);
        if ( ret <= 0 )
                goto err;

        do_read(fd, &plugin_author_len(p), sizeof(plugin_author_len(p)), my_read);
        do_read(fd, &plugin_contact_len(p), sizeof(plugin_contact_len(p)), my_read);
        do_read(fd, &plugin_desc_len(p), sizeof(plugin_desc_len(p)), my_read);
        do_read(fd, &alert_quickmsg_len(alert), sizeof(alert_quickmsg_len(alert)), my_read);
        do_read(fd, &alert_kind(alert), sizeof(alert_kind(alert)), my_read);
        do_read(fd, &alert_count(alert), sizeof(alert_count(alert)), my_read);
        do_read(fd, &alert_time_start(alert), sizeof(alert_time_start(alert)), my_read);
        do_read(fd, &alert_time_end(alert), sizeof(alert_time_end(alert)), my_read);
        do_read(fd, &alert_message_len(alert), sizeof(alert_message_len(alert)), my_read);

        /*
         * Message is a static array.
         */
        if ( alert_message_len(alert) > sizeof(alert_message(alert)) ) {
                log(LOG_ERR, "Buffer overflow attempt against Prelude : %d > %d.\n",
                    alert_message_len(alert), sizeof(alert_message(alert)));
                ret = -1;
                goto err;
        }
        
        do_read(fd, &alert_message(alert), alert_message_len(alert), my_read);
        
        plugin_name(p) = malloc(plugin_name_len(p) + 1);
        if ( ! plugin_name(p) )
                goto err;
        
        ret = socket_read_nowait(fd, plugin_name(p), plugin_name_len(p), my_read);
        if ( ret < 0 ) {
                free(plugin_name(p));
                goto err;
        }
        plugin_name(p)[plugin_name_len(p)] = '\0';

        plugin_author(p) = malloc(plugin_author_len(p) + 1);
        if (! plugin_author(p) ) {
                free(plugin_name(p));
                goto err;
        }

        ret = socket_read_nowait(fd, plugin_author(p), plugin_author_len(p), my_read);
        if ( ret <= 0 ) {
                free(plugin_name(p));
                free(plugin_author(p));
                goto err;
        }
        plugin_author(p)[plugin_author_len(p)] = '\0';
        

        plugin_contact(p) = malloc(plugin_contact_len(p) + 1);
        if ( ! plugin_contact(p) ) {
                free(plugin_name(p));
                free(plugin_author(p));
                goto err;
        }

        ret = socket_read_nowait(fd, plugin_contact(p), plugin_contact_len(p), my_read);
        if ( ret <= 0 ) {
                free(plugin_name(p));
                free(plugin_author(p));
                free(plugin_contact(p));
                goto err;
        }
        plugin_contact(p)[plugin_contact_len(p)] = '\0';
        

        plugin_desc(p) = malloc(plugin_desc_len(p) + 1);
        if ( ! plugin_desc(p) ) {
                free(plugin_name(p));
                free(plugin_author(p));
                free(plugin_contact(p));
                goto err;
        }

        ret = socket_read_nowait(fd, plugin_desc(p), plugin_desc_len(p), my_read);
        if ( ret <= 0 ) {
                free(plugin_name(p));
                free(plugin_author(p));
                free(plugin_contact(p));
                free(plugin_desc(p));
                goto err;
        }
        plugin_desc(p)[plugin_desc_len(p)] = '\0';
        
        alert_quickmsg(alert) = malloc(alert_quickmsg_len(alert) + 1);
        if ( ! alert_quickmsg(alert) ) {
                free(plugin_name(p));
                free(plugin_author(p));
                free(plugin_contact(p));
                free(plugin_desc(p));
                goto err;
        }

        ret = socket_read_nowait(fd, alert_quickmsg(alert), alert_quickmsg_len(alert), my_read);
        if ( ret <= 0 ) {
                free(plugin_name(p));
                free(plugin_author(p));
                free(plugin_contact(p));
                free(plugin_desc(p));
                free(alert_quickmsg(alert));
                goto err;
        }
        alert_quickmsg(alert)[alert_quickmsg_len(alert)] = '\0';
        
        return ret;

 err:
        if ( ret < 0 )
                log(LOG_ERR, "Error reading alert.\n");

        return ret;
}





int alert_read(int sock, alert_t *alert,
               ssize_t (*my_read)(int fd, void *data, size_t size)) 
{
        int ret;
        
        ret = read_alert(sock, alert, my_read);
        if ( ret <= 0 )
                goto err;
        
        return 1;

 err:
        if ( ret < 0 )
                log(LOG_ERR, "Error reading report.\n");
        
        return ret;
}





/*
 *
 */
void alert_free(alert_t *alert, int dfree) 
{
        if ( plugin_name(alert_plugin(alert)) )
                free(plugin_name(alert_plugin(alert)));
        
        if ( plugin_desc(alert_plugin(alert)) )
                free(plugin_desc(alert_plugin(alert)));

        if ( plugin_author(alert_plugin(alert)) )
                free(plugin_author(alert_plugin(alert)));
        
        if ( plugin_contact(alert_plugin(alert)) )
                free(plugin_contact(alert_plugin(alert)));

        if ( alert_quickmsg(alert) )
                free(alert_quickmsg(alert));
}





