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
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "prelude-list.h"
#include "timer.h"
#include "common.h"
#include "config-engine.h"
#include "prelude-io.h"
#include "prelude-message.h"
#include "prelude-client.h"
#include "prelude-client-mgr.h"
#include "prelude-async.h"

#define INITIAL_EXPIRATION_TIME 60
#define MAXIMUM_EXPIRATION_TIME 3600

#define BACKUP_DIR "/var/lib/prelude"



/*
 * This list is in fact a boolean AND of client.
 * When emitting a message, if one of the connection in this
 * list fail, we'll have to consider a OR (if available), or backup
 * our message for later emission.
 */
typedef struct {
        struct list_head list;

        /*
         * If dead is non zero,
         * it mean one of the client in the list is down. 
         */
        unsigned int dead;
        
        prelude_client_mgr_t *parent;
        struct list_head client_list;
} client_list_t;



typedef struct {
        struct list_head list;

        /*
         * Timer for client reconnection.
         */
        prelude_timer_t timer;
        
        /*
         * Pointer on a client object.
         */
        prelude_client_t *client;

        /*
         * Pointer to the parent of this client.
         */
        client_list_t *parent;
} client_t;



struct prelude_client_mgr {
        prelude_io_t *backup_fd;
        struct list_head or_list;
};





static void expand_timeout(prelude_timer_t *timer) 
{
        if ( timer_expire(timer) < MAXIMUM_EXPIRATION_TIME )
                timer_set_expire(timer, timer_expire(timer) * 2);
}



static int broadcast_saved_message(client_list_t *clist, prelude_io_t *fd, size_t size) 
{
        int ret;
        client_t *client;
        struct list_head *tmp;

        log(LOG_INFO, "Flushing saved messages.\n");
        
        list_for_each(tmp, &clist->client_list) {
                client = list_entry(tmp, client_t, list);
                
                ret = lseek(prelude_io_get_fd(fd), 0L, SEEK_SET);
                if ( ret < 0 ) {
                        log(LOG_ERR, "couldn't seek to the begining of the file.\n");
                        return -1;
                }
                
                ret = prelude_client_forward(client->client, fd, size);
                if ( ret < 0 ) {
                        log(LOG_ERR, "error forwarding saved message.\n");
                        return -1;
                }
        }

        return 0;
}




static void flush_backup_if_needed(client_list_t *clist) 
{
        int ret, fd;
        struct stat st;
        prelude_io_t *pio = clist->parent->backup_fd;

        fd = prelude_io_get_fd(pio);
        
        ret = fstat(fd, &st);
        if ( ret < 0 ) {
                log(LOG_ERR, "couldn't stat backup file descriptor.\n");
                return;
        }
        
        if ( st.st_size == 0 )
                return; /* no data saved */
        
        ret = broadcast_saved_message(clist, pio, st.st_size);
        if ( ret < 0 ) {
                log(LOG_ERR, "couldn't broadcast saved message.\n");
                return;
        }
        
        ret = ftruncate(fd, 0);
        if ( ret < 0 ) {
                log(LOG_ERR, "couldn't truncate backup FD to 0 bytes.\n");
                return;
        }

        ret = lseek(fd, 0, SEEK_SET);
        if ( ret < 0 ) {
                log(LOG_ERR, "couldn't seek to the begining of the file.\n");
                return;
        }
}




/*
 * Function called back when one of the client reconnection timer expire.
 */
static void client_timer_expire(void *data) 
{
        int ret;
        client_t *client = data;

        ret = prelude_client_connect(client->client);
        if ( ret < 0 ) {
                /*
                 * Connection failed:
                 * expand the current timeout and reset the timer.
                 */
                expand_timeout(&client->timer);
                timer_reset(&client->timer);

        } else {
                /*
                 * Connection succeed:
                 * Destroy the timer, and if no client in the AND list
                 * is dead, emmit backuped report.
                 */
                timer_destroy(&client->timer);

                
                    //prelude_client_get_connection_infos(client->client);
                
                if ( --client->parent->dead == 0 ) 
                        flush_backup_if_needed(client->parent);
        }
}




/*
 * Separate address and port from a string (x.x.x.x:port).
 */
static void parse_address(char *addr, uint16_t *port) 
{
        char *ptr;
        
        ptr = strchr(addr, ':');
        if ( ! ptr )
                *port = 5554;
        else {
                *ptr = '\0';
                *port = atoi(++ptr);
        }
}




static int add_new_client(client_list_t *clist, char *addr) 
{
        int ret;
        uint16_t port;
        client_t *new;

        new = malloc(sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return -1;
        }
        
        parse_address(addr, &port);
        
        new->parent = clist;
        
        new->client = prelude_client_new(addr, port);
        if ( ! new->client ) {
                free(new);
                return -1;
        }

        timer_set_data(&new->timer, new);
        timer_set_expire(&new->timer, INITIAL_EXPIRATION_TIME);
        timer_set_callback(&new->timer, client_timer_expire);
        
        ret = prelude_client_connect(new->client);
        if ( ret < 0 ) {
                /*
                 * This client couldn't connect, which mean the whole
                 * list (boolean AND) of client won't be used. Initialize the
                 * reconnection timer.
                 */
                clist->dead++;
                timer_init(&new->timer);
        }
        
        list_add_tail(&new->list, &clist->client_list);
        
        return 0;
}




/*
 * Create a list (boolean AND) of client.
 */
static client_list_t *create_client_list(prelude_client_mgr_t *cmgr) 
{
        client_list_t *new;

        new = malloc(sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }
        
        new->dead = 0;
        new->parent = cmgr;
        INIT_LIST_HEAD(&new->client_list);
        
        list_add_tail(&new->list, &cmgr->or_list);
        
        return new;
}



static char *parse_config_string(char **strp) 
{
        char *out, *str = *strp;

        if ( ! *strp ) 
                return NULL;
        
        while ( *str != '\0' && *str == ' ' )
                str++;

        out = str;

        while ( *str != '\0' && *str != ' ' )
                str++;

        if ( *str == ' ' ) {
                *str = '\0';
                *strp = str + 1;
        } else
                *strp = NULL;
        
        return out;
}




/*
 * Parse Manager configuration line:
 * x.x.x.x && y.y.y.y || z.z.z.z
 */
static int parse_config_line(prelude_client_mgr_t *cmgr, char *cfgline) 
{
        int ret;
        char *ptr;
        client_list_t *clist;
                
        clist = create_client_list(cmgr);
        if ( ! clist )
                return -1;

        while ( 1 ) {

                ptr = parse_config_string(&cfgline);                
                if ( ! ptr )
                        break;

                ret = strcmp(ptr, "&&");
                if ( ret == 0 )
                        continue;

                ret = strcmp(ptr, "||");
                if ( ret == 0 ) {
                        /*
                         * We just finished adding a AND list.
                         * if there is backup remaining from a previous session, flush it.
                         */
                        if ( clist->dead == 0 )
                                flush_backup_if_needed(clist);
                        
                        clist = create_client_list(cmgr);
                        if ( ! clist )
                                return -1;

                        continue;
                }
                
                ret = add_new_client(clist, ptr);
                if ( ret < 0 )
                        return -1;
        } 

        return 0;
}



static int setup_backup_fd(prelude_client_mgr_t *new, const char *name) 
{
        int fd;
        char filename[1024];
        
        new->backup_fd = prelude_io_new();
        if (! new->backup_fd ) 
                return -1;
        
        snprintf(filename, sizeof(filename), "%s/%s", BACKUP_DIR, name);
        
        fd = open(filename, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
        if ( fd < 0 ) {
                log(LOG_ERR, "couldn't open %s for writing.\n", filename);
                prelude_io_destroy(new->backup_fd);
                return -1;
        }
        
        prelude_io_set_file_io(new->backup_fd, fd);
        
        return 0;
}



static int broadcast_message(prelude_msg_t *msg, client_list_t *clist)  
{
        int ret;
        client_t *c;
        struct list_head *tmp;
        
        list_for_each(tmp, &clist->client_list) {
                c = list_entry(tmp, client_t, list);
                
                ret = prelude_client_send_msg(c->client, msg);
                if ( ret < 0 ) {
                        clist->dead++;
                        timer_init(&c->timer);
                        return -1;
                }
        }

        return 0;
}



/*
 * Return 0 on sucess, -1 on an already signaled failure,
 * -2 on an already signaled failure.
 */
static int walk_manager_lists(prelude_client_mgr_t *cmgr, prelude_msg_t *msg) 
{
        int ret;
        client_list_t *item;
        struct list_head *tmp;
        
        list_for_each(tmp, &cmgr->or_list) {
                item = list_entry(tmp, client_list_t, list);
                
                /*
                 * There is Manager(s) known to be dead in this list.
                 * No need to try to emmit a message to theses.
                 */                
                if ( item->dead ) {
                        ret = -2;
                        continue;
                }
                
                ret = broadcast_message(msg, item);                
                if ( ret >= 0 )  /* AND of Manager emmission succeed */
                        return 0;
        }

        return -1;
}



/**
 * prelude_client_mgr_broadcast:
 * @cmgr: Pointer on a client manager object.
 * @msg: Pointer on a #prelude_msg_t object.
 *
 * Send the message contained in @msg to all the client
 * in the @cmgr group of client asynchronously.
 */
void prelude_client_mgr_broadcast(prelude_client_mgr_t *cmgr, prelude_msg_t *msg) 
{
        int ret;
        
        ret = walk_manager_lists(cmgr, msg);
        if ( ret == 0 )
                return;
        
        /*
         * This is not good. All of our boolean AND rule for message emission
         * failed. Backup the message.
         */
        if ( ret == -2 )
                log(LOG_INFO, "Manager emmission failed. Enabling failsafe mode.\n");  
      
        ret = prelude_msg_write(msg, cmgr->backup_fd);
        if ( ret < 0 ) 
                log(LOG_ERR, "could't backup message.\n");
}



static void broadcast_async_cb(void *obj, void *data) 
{
        prelude_msg_t *msg = obj;
        prelude_client_mgr_t *cmgr = data;

        prelude_client_mgr_broadcast(cmgr, msg);
        prelude_msg_destroy(msg);
}




/**
 * prelude_client_mgr_broadcast_msg:
 * @cmgr: Pointer on a client manager object.
 * @msg: Pointer on a #prelude_msg_t object.
 *
 * Send the message contained in @msg to all the client
 * in the @cmgr group of client asynchronously. After the
 * request is processed, the @msg message will be freed.
 */
void prelude_client_mgr_broadcast_async(prelude_client_mgr_t *cmgr, prelude_msg_t *msg) 
{
        prelude_async_set_callback((prelude_async_object_t *)msg, &broadcast_async_cb);
        prelude_async_set_data((prelude_async_object_t *)msg, cmgr);
        prelude_async_add((prelude_async_object_t *)msg);
}




/**
 * prelude_client_mgr_get_manager_list:
 * @mgr: Pointer on a #prelude_client_mgr_t object.
 * @cb: callback to call for each manager address.
 *
 * This function will call the provided callback @cb,
 * with every Manager address / port pair contained in the Manager list.
 */
void prelude_client_mgr_get_manager_list(prelude_client_mgr_t *mgr,
                                         int (*cb)(const char *addr, uint16_t port)) 
{
        int ret;
        char *addr;
        uint16_t port;
        client_t *client;
        client_list_t *clist;
        struct list_head *tmp, *tmp2;
        

        list_for_each(tmp, &mgr->or_list) {
                clist = list_entry(tmp, client_list_t, list);
                
                list_for_each(tmp2, &clist->client_list) {

                        client = list_entry(tmp, client_t, list);

                        prelude_client_get_address(client->client, &addr, &port);

                        ret = cb(addr, port);
                        if ( ret < 0 )
                                return;
                }            
        }
}




/**
 * prelude_client_mgr_new:
 * @identifier: Identifier for this clients manager.
 * @cfgline: Manager configuration string.
 *
 * prelude_client_mgr_new() initialize a new Client Manager object.
 * The @identifier argument will be used to identify the backup file associated
 * with this object.
 *
 *
 * Returns: a pointer on a #prelude_client_mgr_t object, or NULL
 * if an error occured.
 */
prelude_client_mgr_t *prelude_client_mgr_new(const char *identifier, const char *cfgline) 
{
        int ret;
        char *dup;
        prelude_client_mgr_t *new;
        
        new = malloc(sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }        
        INIT_LIST_HEAD(&new->or_list);
        
        /*
         * Setup a backup file descriptor for this client Manager.
         * It will be used if a message emmission fail.
         */
        ret = setup_backup_fd(new, identifier);
        if ( ret < 0 ) {
                free(new);
                return NULL;
        }

        dup = strdup(cfgline);
        if ( ! dup ) {
                log(LOG_ERR, "couldn't duplicate string.\n");
                prelude_io_close(new->backup_fd);
                prelude_io_destroy(new->backup_fd);
                free(new);
                return NULL;
        }
        
        ret = parse_config_line(new, dup);
        if ( ret < 0 || list_empty(&new->or_list) ) {
                prelude_io_close(new->backup_fd);
                prelude_io_destroy(new->backup_fd);
                free(new);
                return NULL;
        }

        free(dup);
        
        return new;
}












