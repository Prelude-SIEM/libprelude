/*****
* Copyright (C) 2001, 2002, 2003 Yoann Vandoorselaere <yoann@prelude-ids.org>
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
#include <errno.h>
#include <netinet/in.h> /* required by common.h */
#include <assert.h>

#include "common.h"
#include "prelude-list.h"
#include "timer.h"
#include "prelude-log.h"
#include "config-engine.h"
#include "prelude-io.h"
#include "prelude-message.h"
#include "prelude-message-id.h"
#include "prelude-client.h"
#include "prelude-client-mgr.h"
#include "prelude-async.h"
#include "prelude-path.h"
#include "prelude-getopt.h"
#include "extract.h"

#define INITIAL_EXPIRATION_TIME 60
#define MAXIMUM_EXPIRATION_TIME 3600


#define MAX(x, y) (((x) > (y)) ? (x) : (y))



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
        int use_timer;
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
        /*
         * Theses are two different descriptor for the same file.
         * This is needed so writing FD is open with the O_APPEND flag set,
         * and write occur atomically.
         */
        prelude_io_t *backup_fd_read;
        prelude_io_t *backup_fd_write;
    
        void (*notify_cb)(struct list_head *clist);
        struct list_head all_client;
        struct list_head or_list;

        int nfd;
	fd_set fds;
        prelude_timer_t timer;
};




static int process_request(prelude_client_t *client) 
{
        int ret;
        prelude_msg_t *msg = NULL;
        prelude_msg_status_t status;

        status = prelude_msg_read(&msg, prelude_client_get_fd(client));
        if ( status != prelude_msg_finished )
                return -1;

        if ( prelude_msg_get_tag(msg) != PRELUDE_MSG_OPTION_REQUEST )
                return -1;

        ret = prelude_option_process_request(client, msg);

        prelude_msg_destroy(msg);

        return ret;
}




static void check_for_data_cb(void *arg)
{
	int ret, fd;
        fd_set rfds;
        struct timeval tv;
        struct list_head *tmp;
        prelude_client_t *client;
        prelude_client_mgr_t *mgr = (prelude_client_mgr_t *) arg;

        /*
         * Thread care - in case timers are ran asynchronously.
         *
         * - mgr->nfd and mgr->fds can be read without lock,
         *   as they are only modified at initialisation time or
         *   from within a timer (so it's serialized then).
         *
         * - process_request() should be okay, even if the client is
         *   killed (deconnection) at the same time it's called.
         *
         * - The problem now arise with option callback.
         */
        tv.tv_sec = 0;
        tv.tv_usec = 0;
        rfds = mgr->fds;
        
        timer_reset(&mgr->timer);
        
        ret = select(mgr->nfd, &rfds, NULL, NULL, &tv);
        if ( ret <= 0 )
                return;

        list_for_each(tmp, &mgr->all_client) {
                client = prelude_list_get_object(tmp, prelude_client_t);

                fd = prelude_io_get_fd(prelude_client_get_fd(client));
                
                ret = FD_ISSET(fd, &rfds);
                if ( ! ret )
                        continue;

                process_request(client);
        }
}




static void expand_timeout(prelude_timer_t *timer) 
{
        if ( timer_expire(timer) < MAXIMUM_EXPIRATION_TIME )
                timer_set_expire(timer, timer_expire(timer) * 2);
}


#define other_error -2
#define communication_error -1


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
                        return other_error; 
                }
                
                ret = prelude_client_forward(client->client, fd, size);
                if ( ret < 0 ) {
                        clist->dead++;
                        log(LOG_ERR, "error forwarding saved message.\n");
                        return communication_error;
                }
        }

        log(LOG_INFO, "Flushed %u bytes.\n", size);

        return 0;
}




static int flush_backup_if_needed(client_list_t *clist) 
{
        int ret;
        struct stat st;
        prelude_io_t *pio = clist->parent->backup_fd_read;
        
        ret = fstat(prelude_io_get_fd(pio), &st);
        if ( ret < 0 ) {
                log(LOG_ERR, "couldn't stat backup file descriptor.\n");
                return other_error;
        }
        
        if ( st.st_size == 0 )
                return 0; /* no data saved */
        
        ret = broadcast_saved_message(clist, pio, st.st_size);
        if ( ret < 0 ) {
                log(LOG_ERR, "couldn't broadcast saved message.\n");
                return ret;
        }
        
        ret = ftruncate(prelude_io_get_fd(clist->parent->backup_fd_write), 0);
        if ( ret < 0 ) {
                log(LOG_ERR, "couldn't truncate backup FD to 0 bytes.\n");
                return other_error;
        }

        return 0;
}




/*
 * Function called back when one of the client reconnection timer expire.
 */
static void client_timer_expire(void *data) 
{
        int ret, fd;
        client_t *client = data;
        prelude_client_mgr_t *mgr = client->parent->parent;
        
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
                
                if ( --client->parent->dead == 0 ) {
                        ret = flush_backup_if_needed(client->parent);
                        if ( ret == communication_error ) {
                                timer_init(&client->timer);
                                return;
                        }
                }

                /*
                 * notify the user about a new connection.
                 */
                if ( mgr->notify_cb )
                        mgr->notify_cb(&mgr->all_client);

		/*
		 * put the fd in fdset for read from manager.
		 */
                fd = prelude_io_get_fd(prelude_client_get_fd(client->client));

                FD_SET(fd, &mgr->fds);
                mgr->nfd = MAX(fd + 1, mgr->nfd);
        }
}




/*
 * Separate address and port from a string (x.x.x.x:port).
 */
static void parse_address(char *addr, uint16_t *port) 
{
        char *ptr;
        
        ptr = strrchr(addr, ':');
        if ( ! ptr )
                *port = 5554;
        else {
                *ptr = '\0';
                *port = atoi(++ptr);
        }
}




static int add_new_client(client_list_t *clist, prelude_client_t *client, int use_timer) 
{
        client_t *new;

        new = malloc(sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return -1;
        }

        new->use_timer = use_timer;
        new->parent = clist;
        new->client = client;

        if ( use_timer ) {
                timer_set_data(&new->timer, new);        
                timer_set_expire(&new->timer, INITIAL_EXPIRATION_TIME);
                timer_set_callback(&new->timer, client_timer_expire);
        }
        
        prelude_list_add((prelude_linked_object_t *) new->client, &clist->parent->all_client);
        list_add_tail(&new->list, &clist->client_list);

        return 0;
}




static int create_new_client(client_list_t *clist, char *addr, int type) 
{
        int ret, fd;
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
        
        prelude_client_set_type(new->client, type);
        
        prelude_list_add((prelude_linked_object_t *) new->client, &clist->parent->all_client);

        new->use_timer = 1;
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
	else {
                fd = prelude_io_get_fd(prelude_client_get_fd(new->client));
                FD_SET(fd, &clist->parent->fds);
                clist->parent->nfd = MAX(fd + 1, clist->parent->nfd);
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



static char *parse_config_string(char **line) 
{
        char *out, *str = *line;
        
        if ( ! *line ) 
                return NULL;

        /*
         * Walk until next word.
         */
        while ( *str != '\0' && *str == ' ' ) str++;

        /*
         * save it
         */ 
        out = str;

        /*
         * walk until end of word.
         */
        while ( *str != '\0' && *str != ' ' ) str++;
        
        if ( *str == ' ' ) {
                *str = '\0';
                *line = str + 1;
        }

        else if ( *str == '\0' )                 
                *line = NULL;
        
        return out;
}




/*
 * Parse Manager configuration line:
 * x.x.x.x && y.y.y.y || z.z.z.z
 */
static int parse_config_line(prelude_client_mgr_t *cmgr, char *cfgline, int type) 
{
        int ret;
        char *ptr;
        int working_and = 0;
        client_list_t *clist;

        clist = create_client_list(cmgr);
        if ( ! clist )
                return -1;
        
        while ( 1 ) {                
                ptr = parse_config_string(&cfgline);
                
                /*
                 * If we meet end of line or "||",
                 * it mean we just finished adding a AND list.
                 */
                if ( ! ptr || (ret = strcmp(ptr, "||") == 0) ) {
                        /*
                         * is AND of Manager list true (all connection okay) ?
                         * if it is, flush a potential backup file from previous session.
                         */
                        if ( clist->dead == 0 ) {
                                ret = flush_backup_if_needed(clist);
                                if ( ret != communication_error )
                                        working_and = 1;
                        }
                        
                        /*
                         * end of line ?
                         */
                        if ( ! ptr )
                                break;

                        /*
                         * we met the || operator, prepare a new list. 
                         */
                        clist = create_client_list(cmgr);
                        if ( ! clist )
                                return -1;
                        
                        continue;
                }
                
                ret = strcmp(ptr, "&&");
                if ( ret == 0 )
                        continue;
                
                ret = create_new_client(clist, ptr, type);
                if ( ret < 0 )
                        return -1;
        }

        if ( ! working_and ) 
                log(LOG_INFO, "Can't contact configured Manager - Enabling failsafe mode.\n");
                
        return 0;
}



static void file_error(const char *cfgline) 
{
        log(LOG_INFO, "\nBasic file configuration does not exist. Please run :\n"
            "sensor-adduser --sensorname %s --uid %d\n"
            "program on the sensor host to create an account for this sensor.\n\n"
            
            "Be aware that you should also pass the \"--manager-addr\" option with the\n"
            "manager address as argument. \"sensor-adduser\" should be called for\n"
            "each configured manager address. Here is the configuration line :\n\n%s\n\n", 
            prelude_get_sensor_name(), prelude_get_program_userid(), cfgline);

        exit(1);
}



static int setup_backup_fd(prelude_client_mgr_t *new, const char *cfgline) 
{
        int wfd, rfd;
        char filename[1024];

        prelude_get_backup_filename(filename, sizeof(filename));
                
        new->backup_fd_write = prelude_io_new();
        if (! new->backup_fd_write ) 
                return -1;
        
        new->backup_fd_read = prelude_io_new();
        if (! new->backup_fd_read ) 
                return -1;
        
        /*
         * we want two different descriptor in order to
         * do write atomically (O_APPEND). The other file descriptor has it's
         * own file table, with it's own offset so we can read from it.
         */
        wfd = open(filename, O_WRONLY|O_APPEND, S_IRUSR|S_IWUSR);
        if ( wfd < 0 ) {
                log(LOG_ERR, "couldn't open %s for writing.\n", filename);
                file_error(cfgline);
                return -1;
        }
        
        rfd = open(filename, O_RDONLY);
        if ( rfd < 0 ) {
                log(LOG_ERR, "couldn't open %s for reading.\n", filename);
                file_error(cfgline);
                return -1;
        }
        
        prelude_io_set_sys_io(new->backup_fd_write, wfd);
        prelude_io_set_sys_io(new->backup_fd_read, rfd);
        
        return 0;
}



static void close_backup_fd(prelude_client_mgr_t *mgr) 
{
        prelude_io_close(mgr->backup_fd_read);
        prelude_io_destroy(mgr->backup_fd_read);

        prelude_io_close(mgr->backup_fd_write);
        prelude_io_destroy(mgr->backup_fd_write);
}




static int broadcast_message(prelude_msg_t *msg, client_list_t *clist)  
{
        int ret;
        client_t *c;
        struct list_head *tmp;

        assert(! list_empty(&clist->client_list));
        
        list_for_each(tmp, &clist->client_list) {
                c = list_entry(tmp, client_t, list);
                
                ret = prelude_client_send_msg(c->client, msg);
                if ( ret < 0 ) {
                        clist->dead++;

                        if ( c->use_timer )
                                timer_init(&c->timer);
                        
                        if ( clist->parent->notify_cb )
                                clist->parent->notify_cb(&clist->parent->all_client);

                        return -1;
                }
        }

        return 0;
}



/*
 * Return 0 on sucess, -1 on a new failure,
 * -2 on an already signaled failure.
 */
static int walk_manager_lists(prelude_client_mgr_t *cmgr, prelude_msg_t *msg) 
{
        int ret = 0;
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
                if ( ret == 0 )  /* AND of Manager emmission succeed */
                        return 0;
        }

        return ret;
}




static client_t *search_client(prelude_client_mgr_t *mgr, prelude_client_t *client) 
{
        client_t *c;
        client_list_t *clist;
        struct list_head *tmp, *tmp2;
        
        list_for_each(tmp, &mgr->or_list) {
                clist = list_entry(tmp, client_list_t, list);
                
                list_for_each(tmp2, &clist->client_list) {
                        c = list_entry(tmp2, client_t, list);
                                                
                        if ( c->client == client )
                                return c;
                }
        }

        return NULL;
}




/**
 * prelude_client_mgr_broadcast:
 * @cmgr: Pointer on a client manager object.
 * @msg: Pointer on a #prelude_msg_t object.
 *
 * Send the message contained in @msg to all the client.
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
        if ( ret == -1 )
                log(LOG_INFO, "Manager emmission failed. Enabling failsafe mode.\n");  
      
        ret = prelude_msg_write(msg, cmgr->backup_fd_write);
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




static prelude_client_mgr_t *client_mgr_new(void) 
{
        prelude_client_mgr_t *new;

        new = malloc(sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        new->notify_cb = NULL;
        INIT_LIST_HEAD(&new->or_list);
        INIT_LIST_HEAD(&new->all_client);
        
	FD_ZERO(&new->fds);
	new->nfd = 0;

        return new;
}





/**
 * prelude_client_mgr_new:
 * @type: type of the manager to add.
 * @cfgline: Manager configuration string.
 *
 * prelude_client_mgr_new() initialize a new Client Manager object.
 * The @filename argument will be the backup file associated with this object.
 *
 * Returns: a pointer on a #prelude_client_mgr_t object, or NULL
 * if an error occured.
 */
prelude_client_mgr_t *prelude_client_mgr_new(int type, const char *cfgline) 
{
        int ret;
        char *dup;
        prelude_client_mgr_t *new;
        
        new = client_mgr_new();
        if ( ! new )
                return NULL;
        
        /*
         * Setup a backup file descriptor for this client Manager.
         * It will be used if a message emmission fail.
         */
        ret = setup_backup_fd(new, cfgline);
        if ( ret < 0 ) {
                free(new);
                return NULL;
        }

        dup = strdup(cfgline);
        if ( ! dup ) {
                log(LOG_ERR, "couldn't duplicate string.\n");
                close_backup_fd(new);
                free(new);
                return NULL;
        }
        
        ret = parse_config_line(new, dup, type);
        if ( ret < 0 || list_empty(&new->or_list) ) {
                close_backup_fd(new);
                free(new);
                return NULL;
        }
        
        timer_set_data(&new->timer, new);
        timer_set_expire(&new->timer, 1);
        timer_set_callback(&new->timer, check_for_data_cb);
        timer_init(&new->timer);
        
        free(dup);
        
        return new;
}




void prelude_client_mgr_destroy(prelude_client_mgr_t *mgr) 
{
        client_t *client;
        client_list_t *clist;
        struct list_head *tmp, *tmp2, *bkp, *bkp2;

        list_for_each_safe(tmp, bkp, &mgr->or_list) {
                clist = list_entry(tmp, client_list_t, list);
                
                list_for_each_safe(tmp2, bkp2, &clist->client_list) {
                        client = list_entry(tmp2, client_t, list);
                        
                        prelude_client_destroy(client->client);
                        free(client);
                }

                free(clist);
        }
        
        prelude_io_close(mgr->backup_fd_read);
        prelude_io_close(mgr->backup_fd_write);
        
        free(mgr);
}



prelude_client_t *prelude_client_mgr_search_client(prelude_client_mgr_t *mgr, const char *addr, int type) 
{
        int ret;
        struct list_head *tmp;
        prelude_client_t *client;

        if ( ! mgr )
                return NULL;
        
        list_for_each(tmp, &mgr->all_client) {
                client = prelude_list_get_object(tmp, prelude_client_t);
                
                if ( type != prelude_client_get_type(client) )
                        continue;
                
                ret = strcmp(addr, prelude_client_get_daddr(client));
                if ( ret == 0 ) 
                        return client;
        }

        return NULL;
}




int prelude_client_mgr_add_client(prelude_client_mgr_t **mgr_ptr, prelude_client_t *client, int use_timer) 
{
        int ret;
        client_list_t *clist;
        prelude_client_mgr_t *mgr;
        
        if ( ! *mgr_ptr ) {
                mgr = client_mgr_new();
                if ( ! mgr )
                        return -1;
                
                *mgr_ptr = mgr;
                
                timer_set_expire(&mgr->timer, 1);
                timer_set_data(&mgr->timer, mgr);
                timer_set_callback(&mgr->timer, check_for_data_cb);
                timer_init(&mgr->timer);
                
        } else 
                mgr = *mgr_ptr;
        
        clist = create_client_list(mgr);
        if ( ! clist ) 
                return -1;
        
        ret = add_new_client(clist, client, use_timer);
        if ( ret < 0 ) {
                free(clist);
                return -1;
        }
                
        ret = setup_backup_fd(mgr, NULL);
        if ( ret < 0 ) {
                if ( ! *mgr_ptr )
                        free(mgr);
                
                return -1;
        }

        return 0;
}





void prelude_client_mgr_notify_connection(prelude_client_mgr_t *mgr, void (*callback)(struct list_head *clist)) 
{
        mgr->notify_cb = callback;
}




struct list_head *prelude_client_mgr_get_client_list(prelude_client_mgr_t *mgr) 
{
        return &mgr->all_client;
}




int prelude_client_mgr_tell_client_dead(prelude_client_mgr_t *mgr, prelude_client_t *client)
{
        client_t *c;
        
        c = search_client(mgr, client);
        if ( ! c )
                return -1;
        
        c->parent->dead++;

        if ( c->use_timer ) 
                timer_init(&c->timer);
        
        return 0;
}




int prelude_client_mgr_tell_client_alive(prelude_client_mgr_t *mgr, prelude_client_t *client) 
{
        int ret;
        client_t *c;
        
        c = search_client(mgr, client);
        if ( ! c )
                return -1;
        
        if ( c->parent->dead == 0 )
                return 0;

        ret = flush_backup_if_needed(c->parent);
        if ( ret < 0 )
                return -1;
        
        c->parent->dead--;
        
        return 0;
}





