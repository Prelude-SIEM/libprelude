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
#include "common.h"
#include "prelude-timer.h"
#include "prelude-log.h"
#include "prelude-message-id.h"
#include "prelude-async.h"
#include "prelude-client.h"
#include "prelude-option.h"
#include "prelude-option-wide.h"
#include "prelude-failover.h"
#include "prelude-error.h"

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
        prelude_list_t list;
        struct cnx *and;
        
        /*
         * Timer for client reconnection.
         */
        prelude_timer_t timer;
        prelude_bool_t timer_started;
        
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
        
        cnx_list_t *or_list;
        prelude_failover_t *failover;
        
        int nfd;
        fd_set fds;

        char *connection_string;
        prelude_connection_capability_t capability;
        
        prelude_client_profile_t *client_profile;
        prelude_connection_mgr_flags_t flags;
        prelude_bool_t connection_string_changed;

        prelude_bool_t timer_started;
        prelude_timer_t timer;
        prelude_list_t all_cnx;

        void *data;
        
        prelude_connection_mgr_event_t wanted_event;
        
        int (*global_event_handler)(prelude_connection_mgr_t *mgr,
                                    prelude_connection_mgr_event_t event);
        
        int (*event_handler)(prelude_connection_mgr_t *mgr,
                             prelude_connection_mgr_event_t event,
                             prelude_connection_t *connection);
};



static void notify_event(prelude_connection_mgr_t *mgr,
                         prelude_connection_mgr_event_t event,
                         prelude_connection_t *connection)
{
        if ( ! (event & mgr->wanted_event) )
                return;
        
        if ( mgr->event_handler )
                mgr->event_handler(mgr, event, connection);

        if ( mgr->global_event_handler )
                mgr->global_event_handler(mgr, event);
}



static void init_cnx_timer(cnx_t *cnx)
{        
        if ( cnx->parent->parent->flags & PRELUDE_CONNECTION_MGR_FLAGS_RECONNECT ) {
                cnx->timer_started = TRUE;
                prelude_timer_init(&cnx->timer);
        }
}



static void connection_list_destroy(cnx_list_t *clist)
{
        void *bkp;
        cnx_t *cnx;
        
        for ( ; clist != NULL; clist = bkp ) {
                
                for ( cnx = clist->and; cnx != NULL; cnx = bkp ) {
                        bkp = cnx->and;
                        
                        if ( cnx->timer_started ) {
                                cnx->timer_started = FALSE;
                                prelude_timer_destroy(&cnx->timer);
                        }

                        prelude_list_del(&cnx->list);
                        
                        prelude_connection_destroy(cnx->cnx);
                        prelude_failover_destroy(cnx->failover);
                        
                        free(cnx);
                }

                bkp = clist->or;
                free(clist);
        }
}



static void notify_dead(cnx_t *cnx, prelude_error_t error)
{
        int fd;
        cnx_list_t *clist = cnx->parent;
        prelude_connection_mgr_t *mgr = clist->parent;

        log(LOG_INFO, "%s: connection error with %s:%u: %s. Failover enabled.\n",
            prelude_strsource(error), prelude_connection_get_daddr(cnx->cnx),
            prelude_connection_get_dport(cnx->cnx), prelude_strerror(error));
        
        clist->dead++;
        init_cnx_timer(cnx);

        if ( mgr->event_handler && mgr->wanted_event & PRELUDE_CONNECTION_MGR_EVENT_DEAD )
                mgr->event_handler(clist->parent, PRELUDE_CONNECTION_MGR_EVENT_DEAD, cnx->cnx);
                
        fd = prelude_io_get_fd(prelude_connection_get_fd(cnx->cnx));
        FD_CLR(fd, &clist->parent->fds);
}




static void expand_timeout(prelude_timer_t *timer) 
{
        if ( prelude_timer_get_expire(timer) < MAXIMUM_EXPIRATION_TIME )
                prelude_timer_set_expire(timer, prelude_timer_get_expire(timer) * 2);
}



static void check_for_data_cb(void *arg)
{
        cnx_t *cnx;
        fd_set rfds;
	int ret, fd;
        struct timeval tv;
        prelude_list_t *tmp;
        prelude_connection_mgr_event_t global_event = 0;
        prelude_connection_mgr_t *mgr = (prelude_connection_mgr_t *) arg;
        
        tv.tv_sec = 0;
        tv.tv_usec = 0;
        rfds = mgr->fds;

        prelude_timer_reset(&mgr->timer);

        ret = select(mgr->nfd, &rfds, NULL, NULL, &tv);
        if ( ret <= 0 )
                return;

        prelude_list_for_each(tmp, &mgr->all_cnx) {
                cnx = prelude_list_entry(tmp, cnx_t, list);
                
                if ( ! prelude_connection_is_alive(cnx->cnx) )
                        continue;
                
                fd = prelude_io_get_fd(prelude_connection_get_fd(cnx->cnx));
                
                ret = FD_ISSET(fd, &rfds);       
                if ( ! ret )
                        continue;

                global_event |= PRELUDE_CONNECTION_MGR_EVENT_INPUT;
                
                ret = mgr->event_handler(mgr, PRELUDE_CONNECTION_MGR_EVENT_INPUT, cnx->cnx);
                if ( ret < 0 || ! prelude_connection_is_alive(cnx->cnx) ) {
                        global_event |= PRELUDE_CONNECTION_MGR_EVENT_DEAD;
                        notify_dead(cnx, ret);
                }
        }

        if ( global_event & mgr->wanted_event && mgr->global_event_handler )
                mgr->global_event_handler(mgr, global_event);
        
        if ( mgr->connection_string_changed )
                prelude_connection_mgr_init(mgr);
}



static int broadcast_message(prelude_msg_t *msg, cnx_list_t *clist, cnx_t *cnx)
{
        int ret = -1;

        if ( ! cnx )
                return 0;
        
        if ( prelude_connection_is_alive(cnx->cnx) ) {
                
                ret = prelude_connection_send(cnx->cnx, msg);                
                if ( ret < 0 )
                        notify_dead(cnx, ret);
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
        prelude_msg_t *msg;
        size_t totsize = 0;
        ssize_t size, ret = 0;
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
                        ret = prelude_connection_send(cnx->cnx, msg);
                        if ( ret < 0 )
                                notify_dead(cnx, ret);
                }
                
                prelude_msg_destroy(msg);

                if ( ret < 0 )
                        break;
                
                count++;
                totsize += size;
                
        } while ( 1 );

        log(LOG_INFO, "- %s from failover: %u/%u messages flushed (%u bytes).\n",
            (count == available) ? "Recovered" : "Failed recovering", count, available, totsize);
        
        return ret;
}




/*
 * Function called back when one of the client reconnection timer expire.
 */
static void connection_timer_expire(void *data) 
{
        int ret, fd;
        cnx_t *cnx = data;
        prelude_connection_mgr_t *mgr = cnx->parent->parent;
        
        ret = prelude_connection_connect(cnx->cnx, mgr->client_profile, mgr->capability);
        if ( ret < 0 ) {
                prelude_perror(ret, "could not connect to %s:%hu",
                               prelude_connection_get_daddr(cnx->cnx),
                               prelude_connection_get_dport(cnx->cnx));
                
                /*
                 * Connection failed:
                 * expand the current timeout and reset the timer.
                 */
                expand_timeout(&cnx->timer);
                prelude_timer_reset(&cnx->timer);

        } else {
                /*
                 * Connection succeed:
                 * Destroy the timer, and if no client in the AND list
                 * is dead, emit backuped report.
                 */                
                cnx->parent->dead--;
                prelude_timer_destroy(&cnx->timer);

                notify_event(mgr, PRELUDE_CONNECTION_MGR_EVENT_ALIVE, cnx->cnx);
                                
                ret = failover_flush(cnx->failover, NULL, cnx);
                if ( ret < 0 ) 
                        return;
                
                if ( cnx->parent->dead == 0 ) {
                        ret = failover_flush(mgr->failover, cnx->parent, NULL);
                        if ( ret < 0 )
                                return;
                }
                
                prelude_timer_set_expire(&cnx->timer, INITIAL_EXPIRATION_TIME);
                
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




static cnx_t *new_connection(prelude_client_profile_t *cp, cnx_list_t *clist,
                             prelude_connection_t *cnx, prelude_connection_mgr_flags_t flags) 
{
        cnx_t *new;
        int state, fd, ret;
        char dirname[256], buf[256];
        
        new = malloc(sizeof(*new));
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }
        
        new->parent = clist;
        new->timer_started = FALSE;
        
        if ( flags & PRELUDE_CONNECTION_MGR_FLAGS_RECONNECT ) {
                prelude_timer_set_data(&new->timer, new);
                prelude_timer_set_expire(&new->timer, INITIAL_EXPIRATION_TIME);
                prelude_timer_set_callback(&new->timer, connection_timer_expire);
        }
        
        state = prelude_connection_get_state(cnx);
        
        if ( ! (state & PRELUDE_CONNECTION_ESTABLISHED) ) {
                clist->dead++;
                init_cnx_timer(new);
        } else {
                fd = prelude_io_get_fd(prelude_connection_get_fd(cnx));
                FD_SET(fd, &clist->parent->fds);
                clist->parent->nfd = MAX(fd + 1, clist->parent->nfd);
        }
        
        new->cnx = cnx;
        new->and = NULL;

        prelude_client_profile_get_backup_dirname(cp, buf, sizeof(buf));
        snprintf(dirname, sizeof(dirname), "%s/%s:%u", buf,
                 prelude_connection_get_daddr(cnx), prelude_connection_get_dport(cnx));
        
        ret = prelude_failover_new(&new->failover, dirname);
        if ( ret < 0 ) 
                return NULL;
        
        if ( state & PRELUDE_CONNECTION_ESTABLISHED )
                ret = failover_flush(new->failover, NULL, new);
        
        prelude_list_add(&new->list, &clist->parent->all_cnx);

        return new;
}




static cnx_t *new_connection_from_address(prelude_client_profile_t *cp, cnx_list_t *clist,
                                          char *addr, prelude_connection_mgr_flags_t flags) 
{
        int ret;
        cnx_t *new;
        uint16_t port;
        prelude_connection_t *cnx;
        prelude_connection_mgr_event_t event;
        prelude_connection_mgr_t *mgr = clist->parent;
        
        parse_address(addr, &port);
        
        ret = prelude_connection_new(&cnx, addr, port);        
        if ( ret < 0 ) {
                log(LOG_ERR, "%s: %s\n", prelude_strsource(ret), prelude_strerror(ret));
                return NULL;
        }

        event = PRELUDE_CONNECTION_MGR_EVENT_ALIVE;
        
        ret = prelude_connection_connect(cnx, clist->parent->client_profile, clist->parent->capability);
        if ( ret < 0 ) {
                event = PRELUDE_CONNECTION_MGR_EVENT_DEAD;
                prelude_perror(ret, "could not connect to %s:%hu",
                               prelude_connection_get_daddr(cnx),
                               prelude_connection_get_dport(cnx));
        }
        
        new = new_connection(cp, clist, cnx, flags);
        if ( ! new ) {
                log(LOG_ERR, "memory exhausted.\n");
                return NULL;
        }

        if ( mgr->event_handler && mgr->wanted_event & event )
                mgr->event_handler(mgr, event, cnx);
        
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
static int parse_config_line(prelude_connection_mgr_t *cmgr) 
{
        int ret;
        cnx_t **cnx;
        cnx_list_t *clist = NULL;
        char *ptr, *cfgline = cmgr->connection_string;
        
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
                
                *cnx = new_connection_from_address(cmgr->client_profile, clist, ptr, cmgr->flags);
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




static void broadcast_async_cb(void *obj, void *data) 
{
        prelude_msg_t *msg = obj;
        prelude_connection_mgr_t *cmgr = data;

        printf("broadcast async\n");
        
        prelude_connection_mgr_broadcast(cmgr, msg);
        prelude_msg_destroy(msg);
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
        printf("broadcast\n");
        prelude_timer_lock_critical_region();
        walk_manager_lists(cmgr, msg);        
        prelude_timer_unlock_critical_region();
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



int prelude_connection_mgr_init(prelude_connection_mgr_t *new)
{
        int ret;
        cnx_list_t *clist;
        char dirname[512], buf[512];
        
        if ( ! new->failover ) {                
                prelude_client_profile_get_backup_dirname(new->client_profile, buf, sizeof(buf));        
                snprintf(dirname, sizeof(dirname), "%s/global", buf);

                ret = prelude_failover_new(&new->failover, dirname);
                if ( ret < 0 ) {
                        free(new);
                        return -1;
                }
        }
        
        if ( ! new->connection_string_changed || ! new->connection_string )
                return -1;
        
        new->connection_string_changed = FALSE;
        connection_list_destroy(new->or_list);
                
        new->nfd = 0;
        new->or_list = NULL;
        
        ret = parse_config_line(new);
        if ( ret < 0 || ! new->or_list )
                return -1;
        
        for ( clist = new->or_list; clist != NULL; clist = clist->or ) {
                if ( clist->dead )
                        continue;
                
                ret = failover_flush(new->failover, clist, NULL);
                if ( ret == 0 )
                        break;
        }

        if ( new->global_event_handler )
                new->global_event_handler(new, 0);
        
        if ( ret < 0 )
                log(LOG_INFO, "Can't contact configured Manager - Enabling failsafe mode.\n");
        
        prelude_timer_set_data(&new->timer, new);
        prelude_timer_set_expire(&new->timer, 1);
        prelude_timer_set_callback(&new->timer, check_for_data_cb);
                
        if ( new->event_handler && ! new->timer_started ) {                
                new->timer_started = TRUE;
                prelude_timer_init(&new->timer);
        }
        
        return 0;
}



/**
 * prelude_connection_mgr_new:
 * @mgr: Pointer to an address where to store the created #prelude_connection_mgr_t object.
 * @cp: The #prelude_client_profile_t to use for connection.
 * @capability: Capability the connection in this connection-mgr will declare.
 *
 * prelude_connection_mgr_new() initialize a new Connection Manager object.
 *
 * Returns: 0 on success or a negative value if an error occured.
 */
int prelude_connection_mgr_new(prelude_connection_mgr_t **mgr,
                               prelude_client_profile_t *cp,
                               prelude_connection_capability_t capability)
{
        prelude_connection_mgr_t *new;
        
        *mgr = new = calloc(1, sizeof(*new));
        if ( ! new ) 
                return prelude_error_from_errno(errno);
                
        FD_ZERO(&new->fds);
        new->client_profile = cp;
        new->capability = capability;
        new->connection_string_changed = FALSE;
        PRELUDE_INIT_LIST_HEAD(&new->all_cnx);
        
        return 0;
}



void prelude_connection_mgr_destroy(prelude_connection_mgr_t *mgr) 
{        
        if ( mgr->event_handler && mgr->timer_started )
                prelude_timer_destroy(&mgr->timer);
                
        if ( mgr->connection_string )
                free(mgr->connection_string);
        
        connection_list_destroy(mgr->or_list);

        if ( mgr->failover )
                prelude_failover_destroy(mgr->failover);

        free(mgr);
}




int prelude_connection_mgr_add_connection(prelude_connection_mgr_t *mgr, prelude_connection_t *cnx)
{
        cnx_t **c;

        if ( ! mgr->or_list ) {
                mgr->or_list = create_connection_list(mgr);
                if ( ! mgr->or_list ) 
                        return -1;
        }
        
        for ( c = &mgr->or_list->and; (*c); c = &(*c)->and );

        *c = new_connection(mgr->client_profile, mgr->or_list, cnx, mgr->flags);
        if ( ! (*c) )
                return -1;

        mgr->or_list->total++;
        
        return 0;
}



void prelude_connection_mgr_set_global_event_handler(prelude_connection_mgr_t *mgr,
                                                     prelude_connection_mgr_event_t wanted_event,
                                                     int (*callback)(prelude_connection_mgr_t *mgr,
                                                                     prelude_connection_mgr_event_t events)) 
{
        mgr->wanted_event = wanted_event;
        mgr->global_event_handler = callback;
}



void prelude_connection_mgr_set_event_handler(prelude_connection_mgr_t *mgr,
                                              prelude_connection_mgr_event_t wanted_event,
                                              int (*callback)(prelude_connection_mgr_t *mgr,
                                                              prelude_connection_mgr_event_t events,
                                                              prelude_connection_t *cnx))
{
        mgr->wanted_event = wanted_event;
        mgr->event_handler = callback;
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
        init_cnx_timer(c);
        
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
        if ( ret < 0 )
                return -1;

        if ( --c->parent->dead == 0 ) {
                ret = failover_flush(mgr->failover, c->parent, NULL);
                if ( ret < 0 )
                        return -1;
        }
        
        return 0;
}



int prelude_connection_mgr_set_connection_string(prelude_connection_mgr_t *mgr, const char *cfgstr)
{
        char *new;
        
        new = strdup(cfgstr);
        if ( ! new )
                return prelude_error_from_errno(errno);

        if ( mgr->connection_string )
                free(mgr->connection_string);
        
        mgr->connection_string = new;
        mgr->connection_string_changed = TRUE;

        return 0;
}



const char *prelude_connection_mgr_get_connection_string(prelude_connection_mgr_t *mgr)
{
        return mgr->connection_string;
}



void prelude_connection_mgr_set_flags(prelude_connection_mgr_t *mgr, prelude_connection_mgr_flags_t flags)
{
        mgr->flags = flags;
}



int prelude_connection_mgr_recv(prelude_connection_mgr_t *mgr, prelude_msg_t **out, int timeout) 
{
        cnx_t *cnx;
        int ret, fd;
        fd_set rfds;
        prelude_list_t *tmp;
        struct timeval tv, *tvptr = NULL;
        
        if ( timeout >= 0 ) {
                tvptr = &tv;
                tv.tv_sec = timeout;
                tv.tv_usec = 0;
        }

        rfds = mgr->fds;
        
        ret = select(mgr->nfd, &rfds, NULL, NULL, tvptr);
        if ( ret < 0 )
                return prelude_error_from_errno(ret);

        else if ( ret == 0 )
                return 0;
        
        prelude_list_for_each(tmp, &mgr->all_cnx) {
                cnx = prelude_list_entry(tmp, cnx_t, list);

                if ( ! prelude_connection_is_alive(cnx->cnx) )
                        continue;
                                
                fd = prelude_io_get_fd(prelude_connection_get_fd(cnx->cnx));
                
                ret = FD_ISSET(fd, &rfds);                
                if ( ! ret )
                        continue;

                *out = NULL;
                
                ret = prelude_msg_read(out, prelude_connection_get_fd(cnx->cnx));
                if ( ret < 0 )
                        FD_CLR(fd, &rfds);
                
                return ret;
        }

        return -1;
}



void prelude_connection_mgr_set_data(prelude_connection_mgr_t *mgr, void *data)
{
        mgr->data = data;
}


void *prelude_connection_mgr_get_data(prelude_connection_mgr_t *mgr)
{
        return mgr->data;
}
