/*****
*
* Copyright (C) 2001-2004 Yoann Vandoorselaere <yoann@prelude-ids.org>
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

/*
 * needed for 64 bits file offset.
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h> /* required by common.h */
#include <assert.h>

#include "libmissing.h"
#include "prelude-inttypes.h"
#include "common.h"
#include "prelude-list.h"
#include "prelude-linked-object.h"
#include "timer.h"
#include "prelude-log.h"
#include "config-engine.h"
#include "prelude-io.h"
#include "prelude-message.h"
#include "prelude-message-id.h"
#include "prelude-async.h"
#include "prelude-client.h"
#include "prelude-getopt.h"
#include "prelude-getopt-wide.h"
#include "prelude-failover.h"
#include "extract.h"

#define other_error -2
#define communication_error -1

#define INITIAL_EXPIRATION_TIME 10
#define MAXIMUM_EXPIRATION_TIME 3600


#define MAX(x, y) (((x) > (y)) ? (x) : (y))



/*
 * This list is in fact a boolean AND of client.
 * When emitting a message, if one of the connection in this
 * list fail, we'll have to consider a OR (if available), or backup
 * our message for later emission.
 */
typedef struct cnx_list {
        struct cnx *and;
        struct cnx_list *or;
        
        /*
         * If dead is non zero,
         * it mean one of the client in the list is down. 
         */
        unsigned int dead;
        unsigned int total;
        
        prelude_connection_mgr_t *parent;
} cnx_list_t;



typedef struct cnx {
        struct cnx *and;
        
        prelude_list_t list;

        /*
         * Timer for client reconnection.
         */
        int use_timer;
        prelude_timer_t timer;
        
        prelude_failover_t *failover;
        
        /*
         * Pointer on a client object.
         */
        prelude_connection_t *cnx;

        /*
         * Pointer to the parent of this client.
         */
        cnx_list_t *parent;
} cnx_t;



struct prelude_connection_mgr {

        int dead;
        
        void (*notify_cb)(prelude_list_t *clist);
        
        cnx_list_t *or_list;
        prelude_failover_t *failover;
        
        int nfd;
        fd_set fds;
        prelude_timer_t timer;

        prelude_list_t all_cnx;
};




static void notify_dead(cnx_t *cnx)
{
        int fd;
        cnx_list_t *clist = cnx->parent;

        log(LOG_INFO, "- connection error with %s:%u. Failover enabled.\n",
            prelude_connection_get_daddr(cnx->cnx), prelude_connection_get_dport(cnx->cnx));

        clist->dead++;

        if ( cnx->use_timer )
                timer_init(&cnx->timer);
        
        if ( clist->parent->notify_cb )
                clist->parent->notify_cb(&clist->parent->all_cnx);

        fd = prelude_io_get_fd(prelude_connection_get_fd(cnx->cnx));
        FD_CLR(fd, &clist->parent->fds);
}




static void expand_timeout(prelude_timer_t *timer) 
{
        if ( timer_expire(timer) < MAXIMUM_EXPIRATION_TIME )
                timer_set_expire(timer, timer_expire(timer) * 2);
}




static int process_request(prelude_connection_t *cnx) 
{
        uint8_t tag;
        prelude_io_t *fd;
        prelude_msg_t *msg = NULL;
        prelude_msg_status_t status;

        fd = prelude_connection_get_fd(cnx);
        
        status = prelude_msg_read(&msg, fd);
        if ( status != prelude_msg_finished )
                return -1;

        tag = prelude_msg_get_tag(msg);
        
        if ( tag == PRELUDE_MSG_OPTION_REQUEST )
                prelude_option_process_request(prelude_connection_get_client(cnx), fd, msg);
        
        prelude_msg_destroy(msg);

        return 0;
}




static void check_for_data_cb(void *arg)
{
        fd_set rfds;
	int ret, fd;
        struct timeval tv;
        prelude_list_t *tmp;
        prelude_connection_t *cnx;
        prelude_connection_mgr_t *mgr = (prelude_connection_mgr_t *) arg;

        /*
         * Thread care - in case timers are ran asynchronously.
         *
         * - mgr->nfd and mgr->fds can be read without lock,
         *   as they are only modified at initialisation time or
         *   from within a timer (so it's serialized then).
         *
         * - process_request() should be okay, even if the connection is
         *   killed (disconnection) at the same time it's called.
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

        prelude_list_for_each(tmp, &mgr->all_cnx) {
                cnx = prelude_linked_object_get_object(tmp, prelude_connection_t);
                
                if ( ! (prelude_connection_get_state(cnx) & PRELUDE_CONNECTION_ESTABLISHED) )
                        continue;
                
                fd = prelude_io_get_fd(prelude_connection_get_fd(cnx));
                
                ret = FD_ISSET(fd, &rfds);
                if ( ! ret )
                        continue;
                
                ret = process_request(cnx);                
                if ( ret < 0 )
                        FD_CLR(fd, &mgr->fds);
        }
}





static int broadcast_message(prelude_msg_t *msg, cnx_list_t *clist, cnx_t *cnx)
{
        int ret = -1;

        if ( ! cnx )
                return 0;
        
        if ( prelude_connection_get_state(cnx->cnx) & PRELUDE_CONNECTION_ESTABLISHED ) {
                ret = prelude_connection_send_msg(cnx->cnx, msg);                
                if ( ret < 0 ) 
                        notify_dead(cnx);
        }

        if ( ret < 0 ) 
                prelude_failover_save_msg(cnx->failover, msg);
        
        return broadcast_message(msg, clist, cnx->and);
}



/*
 * Return 0 on sucess, -1 on a new failure,
 * -2 on an already signaled failure.
 */
static int walk_manager_lists(prelude_connection_mgr_t *cmgr, prelude_msg_t *msg) 
{
        int ret = 0;
        cnx_list_t *or;
        
        for ( or = cmgr->or_list; or != NULL; or = or->or ) {
                
                if ( or->dead == or->total ) {
                        ret = -2;
                        continue;
                }

                ret = broadcast_message(msg, or, or->and);                
                if ( ret == 0 )  /* AND of Manager emission succeed */
                        return 0;
        }
        
        prelude_failover_save_msg(cmgr->failover, msg);
        
        return ret;
}



static int failover_flush(prelude_failover_t *failover, cnx_list_t *clist, cnx_t *cnx)
{
        char name[128];
        ssize_t size, ret;
        prelude_msg_t *msg;
        size_t totsize = 0;
        unsigned int available, count = 0;
        
        available = prelude_failover_get_available_msg_count(failover);
        if ( ! available ) 
                return 0;

        if ( clist )
                snprintf(name, sizeof(name), "any");
        else
                snprintf(name, sizeof(name), "%s:%u",
                         prelude_connection_get_daddr(cnx->cnx), prelude_connection_get_dport(cnx->cnx));
        
        log(LOG_INFO, "- Flushing %u message to %s manager (%u erased due to quota)...\n",
            available, name, prelude_failover_get_deleted_msg_count(failover));

        do {
                size = prelude_failover_get_saved_msg(failover, &msg);
                if ( size < 0 )
                        continue;
				
                if ( size == 0 )
                        break;

                if ( clist )
                        ret = broadcast_message(msg, clist, clist->and);
                else {
                        ret = prelude_connection_send_msg(cnx->cnx, msg);
                        if ( ret < 0 )
                                notify_dead(cnx);
                }
                
                prelude_msg_destroy(msg);

                if ( ret < 0 )
                        break;
                
                count++;
                totsize += size;
                
        } while ( 1 );

        log(LOG_INFO, "- %s from failover: %u/%u messages flushed (%u bytes).\n",
            (count == available) ? "Recovered" : "Failed recovering", count, available, totsize);
        
        return 0;
}




/*
 * Function called back when one of the client reconnection timer expire.
 */
static void connection_timer_expire(void *data) 
{
        int ret, fd;
        cnx_t *cnx = data;
        prelude_connection_mgr_t *mgr = cnx->parent->parent;
                
        ret = prelude_connection_connect(cnx->cnx);
        if ( ret < 0 ) {
                /*
                 * Connection failed:
                 * expand the current timeout and reset the timer.
                 */
                expand_timeout(&cnx->timer);
                timer_reset(&cnx->timer);

        } else {
                /*
                 * Connection succeed:
                 * Destroy the timer, and if no client in the AND list
                 * is dead, emit backuped report.
                 */
                cnx->parent->dead--;
                timer_destroy(&cnx->timer);
                
                if ( mgr->notify_cb )
                        mgr->notify_cb(&mgr->all_cnx);
                
                ret = failover_flush(cnx->failover, NULL, cnx);
                if ( ret == communication_error ) 
                        return;
                
                if ( cnx->parent->dead == 0 ) {
                        ret = failover_flush(mgr->failover, cnx->parent, NULL);
                        if ( ret == communication_error )
                                return;
                }
                
                timer_set_expire(&cnx->timer, INITIAL_EXPIRATION_TIME);
                
		/*
		 * put the fd in fdset for read from manager.
		 */
                fd = prelude_io_get_fd(prelude_connection_get_fd(cnx->cnx));

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




static cnx_t *new_connection(prelude_client_t *client, cnx_list_t *clist,
                             prelude_connection_t *cnx, int use_timer) 
{
        cnx_t *new;
        int state, fd, ret;
        char dirname[256], buf[256];
        
        new = malloc(sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }
        
        if ( use_timer ) {
                timer_set_data(&new->timer, new);        
                timer_set_expire(&new->timer, INITIAL_EXPIRATION_TIME);
                timer_set_callback(&new->timer, connection_timer_expire);
        }

        state = prelude_connection_get_state(cnx);
        
        if ( ! (state & PRELUDE_CONNECTION_ESTABLISHED) ) {
                clist->dead++;
                if ( use_timer ) timer_init(&new->timer);
        } else {
                fd = prelude_io_get_fd(prelude_connection_get_fd(cnx));
                FD_SET(fd, &clist->parent->fds);
                clist->parent->nfd = MAX(fd + 1, clist->parent->nfd);
        }
        
        new->cnx = cnx;
        new->and = NULL;
        new->parent = clist;
        new->use_timer = use_timer;

        prelude_client_get_backup_filename(client, buf, sizeof(buf));
        snprintf(dirname, sizeof(dirname), "%s/%s:%u", buf,
                 prelude_connection_get_daddr(cnx), prelude_connection_get_dport(cnx));
        
        new->failover = prelude_failover_new(dirname);
        if ( ! new->failover ) 
                return NULL;
        
        if ( state & PRELUDE_CONNECTION_ESTABLISHED )
                ret = failover_flush(new->failover, NULL, new);
        
        prelude_linked_object_add((prelude_linked_object_t *) new->cnx, &clist->parent->all_cnx);

        return new;
}




static cnx_t *new_connection_from_address(prelude_client_t *client, cnx_list_t *clist, char *addr) 
{
        int ret;
        cnx_t *new;
        uint16_t port;
        prelude_connection_t *cnx;
        
        parse_address(addr, &port);
        
        cnx = prelude_connection_new(client, addr, port);
        if ( ! cnx ) 
                return NULL;

        ret = prelude_connection_connect(cnx);
        
        new = new_connection(client, clist, cnx, 1);
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }
        
        return new;
}




/*
 * Create a list (boolean AND) of connection.
 */
static cnx_list_t *create_connection_list(prelude_connection_mgr_t *cmgr) 
{
        cnx_list_t *new;

        new = calloc(1, sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        new->parent = cmgr;
        
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
static int parse_config_line(prelude_client_t *client, prelude_connection_mgr_t *cmgr, char *cfgline) 
{
        int ret;
        char *ptr;
        cnx_t **cnx;
        cnx_list_t *clist = NULL;

        clist = cmgr->or_list = create_connection_list(cmgr);
        if ( ! clist )
                return -1;

        cnx = &clist->and;
        
        while ( 1 ) {                
                ptr = parse_config_string(&cfgline);
                
                /*
                 * If we meet end of line or "||",
                 * it mean we just finished adding a AND list.
                 */
                if ( ! ptr || (ret = strcmp(ptr, "||") == 0) ) {
                        /*
                         * is AND of Manager list true (all connection okay) ?
                         */
                        if ( clist->dead != 0 )
                                cmgr->dead++;
                                                
                        /*
                         * end of line ?
                         */
                        if ( ! ptr )
                                break;

                        /*
                         * we met the || operator, prepare a new list. 
                         */
                        clist->or = create_connection_list(cmgr);
                        if ( ! clist )
                                return -1;

                        clist = clist->or;
                        cnx = &clist->and;
                        continue;
                }
                
                ret = strcmp(ptr, "&&");
                if ( ret == 0 )
                        continue;
                
                *cnx = new_connection_from_address(client, clist, ptr);
                if ( ! *cnx )
                        return -1;

                clist->total++;
                cnx = &(*cnx)->and;
        }
                
        return 0;
}




static cnx_t *search_cnx(prelude_connection_mgr_t *mgr, prelude_connection_t *cnx) 
{
        cnx_t *c;
        cnx_list_t *clist;

        for ( clist = mgr->or_list; clist != NULL; clist = clist->or ) {

                for ( c = clist->and; c != NULL; c = c->and ) {

                        if ( c->cnx == cnx )
                                return c;
                }
        }

        return NULL;
}




/**
 * prelude_connection_mgr_broadcast:
 * @cmgr: Pointer on a client manager object.
 * @msg: Pointer on a #prelude_msg_t object.
 *
 * Send the message contained in @msg to all the client.
 */
void prelude_connection_mgr_broadcast(prelude_connection_mgr_t *cmgr, prelude_msg_t *msg) 
{
        timer_lock_critical_region();
        walk_manager_lists(cmgr, msg);        
        timer_unlock_critical_region();
}




static void broadcast_async_cb(void *obj, void *data) 
{
        prelude_msg_t *msg = obj;
        prelude_connection_mgr_t *cmgr = data;

        prelude_connection_mgr_broadcast(cmgr, msg);
        prelude_msg_destroy(msg);
}



/**
 * prelude_connection_mgr_broadcast_msg:
 * @cmgr: Pointer on a client manager object.
 * @msg: Pointer on a #prelude_msg_t object.
 *
 * Send the message contained in @msg to all the client
 * in the @cmgr group of client asynchronously. After the
 * request is processed, the @msg message will be freed.
 */
void prelude_connection_mgr_broadcast_async(prelude_connection_mgr_t *cmgr, prelude_msg_t *msg) 
{
        prelude_async_set_callback((prelude_async_object_t *) msg, &broadcast_async_cb);
        prelude_async_set_data((prelude_async_object_t *) msg, cmgr);
        prelude_async_add((prelude_async_object_t *) msg);
}




static prelude_connection_mgr_t *connection_mgr_new(prelude_client_t *client) 
{
        char dirname[256], buf[256];
        prelude_connection_mgr_t *new;
        
        new = malloc(sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }
        
        prelude_client_get_backup_filename(client, buf, sizeof(buf));        
        snprintf(dirname, sizeof(dirname), "%s/global", buf);
        
        new->failover = prelude_failover_new(dirname);
        if ( ! new->failover ) {
                free(new);
                return NULL;
        }

        new->nfd = 0;
        FD_ZERO(&new->fds);
	
        new->or_list = NULL;
        new->notify_cb = NULL;
        PRELUDE_INIT_LIST_HEAD(&new->all_cnx);

        return new;
}





/**
 * prelude_connection_mgr_new:
 * @client: Use of this object.
 * @cfgline: Manager configuration string.
 *
 * prelude_connection_mgr_new() initialize a new Connection Manager object.
 *
 * Returns: a pointer on a #prelude_connection_mgr_t object, or NULL
 * if an error occured.
 */
prelude_connection_mgr_t *prelude_connection_mgr_new(prelude_client_t *client, const char *cfgline) 
{
        int ret;
        char *dup;
        cnx_list_t *clist;
        prelude_connection_mgr_t *new;
        
        new = connection_mgr_new(client);
        if ( ! new )
                return NULL;

        dup = strdup(cfgline);
        if ( ! dup ) {
                log(LOG_ERR, "couldn't duplicate string.\n");
                free(new);
                return NULL;
        }
        
        ret = parse_config_line(client, new, dup);
        if ( ret < 0 || ! new->or_list ) {
                free(new);
                return NULL;
        }

        for ( clist = new->or_list; clist != NULL; clist = clist->or ) {
                if ( clist->dead )
                        continue;
                
                ret = failover_flush(new->failover, clist, NULL);
                if ( ret == 0 )
                        break;
        }
        
        if ( ret < 0 )
                log(LOG_INFO, "Can't contact configured Manager - Enabling failsafe mode.\n");

        timer_set_data(&new->timer, new);
        timer_set_expire(&new->timer, 1);
        timer_set_callback(&new->timer, check_for_data_cb);
        timer_init(&new->timer);
        
        free(dup);
        
        return new;
}




void prelude_connection_mgr_destroy(prelude_connection_mgr_t *mgr) 
{
        void *bkp;
        cnx_t *cnx;
        cnx_list_t *clist;
        
        for ( clist = mgr->or_list; clist != NULL; clist = bkp ) {
                
                for ( cnx = clist->and; cnx != NULL; cnx = bkp ) {
                        bkp = cnx->and;
                        
                        prelude_connection_destroy(cnx->cnx);
                        free(cnx);
                }

                bkp = clist->or;
                free(clist);
        }

        free(mgr);
}



prelude_connection_t *prelude_connection_mgr_search_connection(prelude_connection_mgr_t *mgr, const char *addr)
{
        int ret;
        prelude_list_t *tmp;
        prelude_connection_t *cnx;
        
        if ( ! mgr )
                return NULL;
        
        prelude_list_for_each(tmp, &mgr->all_cnx) {
                cnx = prelude_linked_object_get_object(tmp, prelude_connection_t);

                ret = strcmp(addr, prelude_connection_get_daddr(cnx));
                if ( ret == 0 ) 
                        return cnx;
        }

        return NULL;
}




int prelude_connection_mgr_add_connection(prelude_connection_mgr_t **mgr, prelude_connection_t *cnx, int use_timer) 
{
        cnx_t **c;
        cnx_list_t *clist;
        
        if ( ! *mgr ) {

                *mgr = connection_mgr_new(prelude_connection_get_client(cnx));
                if ( ! *mgr )
                        return -1;

                timer_set_expire(&(*mgr)->timer, 1);
                timer_set_data(&(*mgr)->timer, *mgr);
                timer_set_callback(&(*mgr)->timer, check_for_data_cb);

                if ( use_timer )
                        timer_init(&(*mgr)->timer);
        }
        
        clist = create_connection_list(*mgr);
        if ( ! clist ) 
                return -1;

        for ( c = &clist->and; (*c); c = &(*c)->and );

        *c = new_connection(prelude_connection_get_client(cnx), clist, cnx, use_timer);
        
        if ( ! (*c) ) {
                free(clist);
                return -1;
        }

        (*mgr)->or_list = clist;
        
        return 0;
}





void prelude_connection_mgr_notify_connection(prelude_connection_mgr_t *mgr, void (*callback)(prelude_list_t *clist)) 
{
        mgr->notify_cb = callback;
}




prelude_list_t *prelude_connection_mgr_get_connection_list(prelude_connection_mgr_t *mgr) 
{
        return &mgr->all_cnx;
}




int prelude_connection_mgr_tell_connection_dead(prelude_connection_mgr_t *mgr, prelude_connection_t *cnx)
{
        cnx_t *c;
        
        c = search_cnx(mgr, cnx);
        if ( ! c )
                return -1;
        
        c->parent->dead++;

        if ( c->use_timer ) 
                timer_init(&c->timer);
        
        return 0;
}




int prelude_connection_mgr_tell_connection_alive(prelude_connection_mgr_t *mgr, prelude_connection_t *cnx) 
{
        int ret;
        cnx_t *c;
        
        c = search_cnx(mgr, cnx);
        if ( ! c )
                return -1;
        
        if ( c->parent->dead == 0 )
                return 0;

        ret = failover_flush(c->failover, NULL, c);
        if ( ret == communication_error ) 
                return -1;

        if ( --c->parent->dead == 0 ) {
                ret = failover_flush(mgr->failover, c->parent, NULL);
                if ( ret == communication_error ) 
                        return -1;
        }
        
        return 0;
}
