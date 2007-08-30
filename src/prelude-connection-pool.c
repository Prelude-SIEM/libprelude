/*****
*
* Copyright (C) 2001-2005,2006,2007 PreludeIDS Technologies. All Rights Reserved.
* Author: Yoann Vandoorselaere <yoann.v@prelude-ids.com>
*
* This file is part of the Prelude library.
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

#include "config.h"
#include "libmissing.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>

#include "common.h"
#include "prelude-timer.h"
#include "prelude-log.h"
#include "prelude-message-id.h"
#include "prelude-async.h"
#include "prelude-client.h"
#include "prelude-option.h"
#include "prelude-option-wide.h"
#include "prelude-failover.h"

#define PRELUDE_ERROR_SOURCE_DEFAULT PRELUDE_ERROR_SOURCE_CONNECTION_POOL
#include "prelude-error.h"

#define INITIAL_EXPIRATION_TIME 10
#define MAXIMUM_EXPIRATION_TIME 3600




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
         * it means one of the client in the list is down.
         */
        unsigned int dead;
        unsigned int total;

        prelude_connection_pool_t *parent;
} cnx_list_t;



typedef struct cnx {
        prelude_list_t list;
        struct cnx *and;

        /*
         * Timer for client reconnection.
         */
        prelude_timer_t timer;
        prelude_failover_t *failover;
        prelude_bool_t is_dead;

        /*
         * Pointer on a client object.
         */
        prelude_connection_t *cnx;

        /*
         * Pointer to the parent of this client.
         */
        cnx_list_t *parent;
} cnx_t;



struct prelude_connection_pool {

        cnx_list_t *or_list;
        prelude_failover_t *failover;

        int nfd;
        fd_set fds;
        int refcount;

        char *connection_string;
        prelude_connection_permission_t permission;

        prelude_client_profile_t *client_profile;
        prelude_connection_pool_flags_t flags;
        prelude_bool_t connection_string_changed;

        prelude_timer_t timer;
        prelude_list_t all_cnx;

        /*
         * FIXME: do we really want this?
         */
        prelude_list_t int_cnx;

        void *data;

        prelude_connection_pool_event_t wanted_event;

        int (*global_event_handler)(prelude_connection_pool_t *pool,
                                    prelude_connection_pool_event_t event);

        int (*event_handler)(prelude_connection_pool_t *pool,
                             prelude_connection_pool_event_t event,
                             prelude_connection_t *connection);
};



static int do_send(prelude_connection_t *conn, prelude_msg_t *msg)
{
        int ret;

        /*
         * handle EAGAIN in case the caller use non blocking IO.
         */
        do {
                ret = prelude_connection_send(conn, msg);
        } while ( ret < 0 && prelude_error_get_code(ret) == PRELUDE_ERROR_EAGAIN );

        return ret;
}



static int get_connection_backup_path(prelude_connection_t *cn, const char *path, char **out)
{
        int ret;
        char c, buf[512];
        const char *addr;
        prelude_string_t *str;

        ret = prelude_string_new_dup(&str, path);
        if ( ret < 0 )
                return ret;

        prelude_string_cat(str, "/");

        /*
         * FIXME: ideally we should only use peer analyzerid. This would
         * imply creating the failover after the first connection.
         */
        addr = prelude_connection_get_peer_addr(cn);
        if ( ! addr )
                prelude_string_sprintf(str, "%" PRELUDE_PRIu64, prelude_connection_get_peer_analyzerid(cn));

        else {
                snprintf(buf, sizeof(buf), "%s:%u",
                         prelude_connection_get_peer_addr(cn), prelude_connection_get_peer_port(cn));

                while ( (c = *addr++) ) {
                        if ( c == '/' )
                                c = '_';

                        prelude_string_ncat(str, &c, 1);
                }
        }

        ret = prelude_string_get_string_released(str, out);
        prelude_string_destroy(str);

        return ret;
}



static void notify_event(prelude_connection_pool_t *pool,
                         prelude_connection_pool_event_t event,
                         prelude_connection_t *connection)
{
        if ( ! (event & pool->wanted_event) )
                return;

        if ( pool->event_handler )
                pool->event_handler(pool, event, connection);

        if ( pool->global_event_handler )
                pool->global_event_handler(pool, event);
}



static void init_cnx_timer(cnx_t *cnx)
{
        if ( cnx->parent->parent->flags & PRELUDE_CONNECTION_POOL_FLAGS_RECONNECT )
                prelude_timer_init(&cnx->timer);
}



static void connection_list_destroy(cnx_list_t *clist)
{
        void *bkp;
        cnx_t *cnx;

        for ( ; clist != NULL; clist = bkp ) {

                for ( cnx = clist->and; cnx != NULL; cnx = bkp ) {
                        bkp = cnx->and;

                        prelude_timer_destroy(&cnx->timer);

                        prelude_list_del(&cnx->list);

                        prelude_connection_destroy(cnx->cnx);

                        if ( cnx->failover )
                                prelude_failover_destroy(cnx->failover);

                        free(cnx);
                }

                bkp = clist->or;
                free(clist);
        }
}



static void notify_dead(cnx_t *cnx, prelude_error_t error, prelude_bool_t init_time)
{
        int fd;
        prelude_error_code_t code;
        cnx_list_t *clist = cnx->parent;
        prelude_connection_pool_t *pool = clist->parent;

        if ( cnx->is_dead )
                return;

        code = prelude_error_get_code(error);

        if ( ! init_time || code != PRELUDE_ERROR_PROFILE )
                prelude_log(PRELUDE_LOG_WARN, "%sconnection error with %s: %s\n",
                            (pool->flags & PRELUDE_CONNECTION_POOL_FLAGS_FAILOVER) ? "Failover enabled: " : "",
                            prelude_connection_get_peer_addr(cnx->cnx), prelude_strerror(error));

        clist->dead++;
        cnx->is_dead = TRUE;

        init_cnx_timer(cnx);

        if ( pool->event_handler && pool->wanted_event & PRELUDE_CONNECTION_POOL_EVENT_DEAD )
                pool->event_handler(clist->parent, PRELUDE_CONNECTION_POOL_EVENT_DEAD, cnx->cnx);

        if ( ! init_time ) {
                fd = prelude_io_get_fd(prelude_connection_get_fd(cnx->cnx));
                assert(fd < FD_SETSIZE);

                FD_CLR(fd, &clist->parent->fds);
        }
}




static void expand_timeout(prelude_timer_t *timer)
{
        if ( prelude_timer_get_expire(timer) < MAXIMUM_EXPIRATION_TIME )
                prelude_timer_set_expire(timer, prelude_timer_get_expire(timer) * 2);
}



static void check_for_data_cb(void *arg)
{
        prelude_connection_pool_t *pool = arg;

        prelude_connection_pool_check_event(pool, 0, NULL, NULL);
        prelude_timer_reset(&pool->timer);
}


static int failover_save_msg(prelude_failover_t *failover, prelude_msg_t *msg)
{
        int ret;

        ret = prelude_failover_save_msg(failover, msg);
        if ( ret < 0 )
                prelude_log(PRELUDE_LOG_WARN, "failover error: %s.\n", prelude_strerror(ret));

        return ret;
}


static void broadcast_message(prelude_msg_t *msg, cnx_t *cnx)
{
        int ret = -1;

        if ( ! cnx )
                return;

        if ( prelude_connection_is_alive(cnx->cnx) ) {

                ret = do_send(cnx->cnx, msg);
                if ( ret < 0 )
                        notify_dead(cnx, ret, FALSE);
        }

        if ( ret < 0 && cnx->failover )
                failover_save_msg(cnx->failover, msg);

        broadcast_message(msg, cnx->and);
}



/*
 * Returns 0 on sucess, -1 on a new failure,
 * -2 on an already signaled failure.
 */
static int walk_manager_lists(prelude_connection_pool_t *pool, prelude_msg_t *msg)
{
        int ret = 0;
        cnx_list_t *or;

        for ( or = pool->or_list; or != NULL; or = or->or ) {

                /*
                 * if all connections are dead and we have a or, go to next.
                 */
                if ( or->dead == or->total && (pool->flags & PRELUDE_CONNECTION_POOL_FLAGS_FAILOVER) ) {
                        ret = -2;
                        continue;
                }

                broadcast_message(msg, or->and);
                return 0;
        }

        if ( pool->failover )
                failover_save_msg(pool->failover, msg);

        return ret;
}



static int failover_flush(prelude_failover_t *failover, cnx_list_t *clist, cnx_t *cnx)
{
        char name[128];
        prelude_msg_t *msg;
        size_t totsize = 0;
        ssize_t size, ret = 0;
        unsigned int available, count = 0;

        if ( ! failover )
                return 0;

        available = prelude_failover_get_available_msg_count(failover);
        if ( ! available )
                return 0;

        if ( clist )
                snprintf(name, sizeof(name), "any");
        else
                snprintf(name, sizeof(name), "0x%" PRELUDE_PRIx64, prelude_connection_get_peer_analyzerid(cnx->cnx));

        prelude_log(PRELUDE_LOG_INFO,
                    "Flushing %u message to %s (%lu erased due to quota)...\n",
                    available, name, prelude_failover_get_deleted_msg_count(failover));

        do {
                size = prelude_failover_get_saved_msg(failover, &msg);
                if ( size == 0 )
                        break;

                if ( size < 0 ) {
                        prelude_log(PRELUDE_LOG_ERR, "error reading message from failover: %s", prelude_strerror(size));
                        break;
                }

                if ( clist ) {
                        broadcast_message(msg, clist->and);
                        if ( clist->dead )
                                ret = -1;
                } else {
                        ret = do_send(cnx->cnx, msg);
                        if ( ret < 0 ) {
                                notify_dead(cnx, ret, FALSE);
                                if ( cnx->failover )
                                        failover_save_msg(cnx->failover, msg);
                        }
                }

                prelude_msg_destroy(msg);

                if ( ret < 0 )
                        break;

                count++;
                totsize += size;

        } while ( 1 );

        prelude_log(PRELUDE_LOG_WARN, "Failover recovery: %u/%u messages flushed (%" PRELUDE_PRIu64 " bytes).\n",
                    count, available, (uint64_t) totsize);

        return ret;
}




/*
 * Function called back when one of the client reconnection timer expires.
 */
static void connection_timer_expire(void *data)
{
        int ret, fd;
        cnx_t *cnx = data;
        prelude_connection_pool_t *pool = cnx->parent->parent;

        ret = prelude_connection_connect(cnx->cnx, pool->client_profile, pool->permission);
        if ( ret < 0 ) {
                prelude_log(PRELUDE_LOG_WARN, "%sconnection error with %s: %s\n",
                            (pool->flags & PRELUDE_CONNECTION_POOL_FLAGS_FAILOVER) ? "Failover enabled: " : "",
                            prelude_connection_get_peer_addr(cnx->cnx), prelude_strerror(ret));

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
                cnx->is_dead = FALSE;
                prelude_timer_destroy(&cnx->timer);
                prelude_timer_set_expire(&cnx->timer, INITIAL_EXPIRATION_TIME);

                notify_event(pool, PRELUDE_CONNECTION_POOL_EVENT_ALIVE, cnx->cnx);

                ret = failover_flush(cnx->failover, NULL, cnx);
                if ( ret < 0 )
                        return;

                if ( pool->failover && cnx->parent->dead == 0 ) {
                        ret = failover_flush(pool->failover, cnx->parent, NULL);
                        if ( ret < 0 )
                                return;
                }

                /*
                 * put the fd in fdset to read from manager.
                 */
                fd = prelude_io_get_fd(prelude_connection_get_fd(cnx->cnx));
                assert(fd < FD_SETSIZE);

                FD_SET(fd, &pool->fds);
                pool->nfd = MAX(fd + 1, pool->nfd);
        }
}




static int new_connection(cnx_t **ncnx, prelude_client_profile_t *cp, cnx_list_t *clist,
                          prelude_connection_t *cnx, prelude_connection_pool_flags_t flags)
{
        cnx_t *new;
        int state, fd, ret;
        char *dirname, buf[256];

        new = malloc(sizeof(*new));
        if ( ! new )
                return prelude_error_from_errno(errno);

        new->failover = NULL;
        new->parent = clist;
        new->is_dead = FALSE;
        prelude_timer_init_list(&new->timer);

        if ( flags & PRELUDE_CONNECTION_POOL_FLAGS_RECONNECT ) {
                prelude_timer_set_data(&new->timer, new);
                prelude_timer_set_expire(&new->timer, INITIAL_EXPIRATION_TIME);
                prelude_timer_set_callback(&new->timer, connection_timer_expire);
        }

        if ( flags & PRELUDE_CONNECTION_POOL_FLAGS_FAILOVER ) {
                prelude_client_profile_get_backup_dirname(cp, buf, sizeof(buf));

                ret = get_connection_backup_path(cnx, buf, &dirname);
                if ( ret < 0 )
                        return ret;

                ret = prelude_failover_new(&new->failover, dirname);
                free(dirname);
                if ( ret < 0 ) {
                        free(new);
                        return ret;
                }
        }

        state = prelude_connection_get_state(cnx);
        if ( state & PRELUDE_CONNECTION_STATE_ESTABLISHED ) {
                fd = prelude_io_get_fd(prelude_connection_get_fd(cnx));
                assert(fd < FD_SETSIZE);

                FD_SET(fd, &clist->parent->fds);
                clist->parent->nfd = MAX(fd + 1, clist->parent->nfd);
        }

        new->cnx = cnx;
        new->and = NULL;

        if ( state & PRELUDE_CONNECTION_STATE_ESTABLISHED )
                ret = failover_flush(new->failover, NULL, new);

        prelude_list_add(&clist->parent->int_cnx, &new->list);
        prelude_linked_object_add(&clist->parent->all_cnx, (prelude_linked_object_t *) cnx);

        *ncnx = new;

        return 0;
}




static int new_connection_from_address(cnx_t **new,
                                       prelude_client_profile_t *cp, cnx_list_t *clist,
                                       char *addr, prelude_connection_pool_flags_t flags)
{
        int ret, cret;
        prelude_connection_t *cnx;
        prelude_connection_pool_event_t event;
        prelude_connection_pool_t *pool = clist->parent;

        ret = prelude_connection_new(&cnx, addr);
        if ( ret < 0 )
                return ret;

        event = PRELUDE_CONNECTION_POOL_EVENT_ALIVE;

        cret = prelude_connection_connect(cnx, clist->parent->client_profile, clist->parent->permission);
        if ( cret < 0 )
                event = PRELUDE_CONNECTION_POOL_EVENT_DEAD;

        ret = new_connection(new, cp, clist, cnx, flags);
        if ( ret < 0 ) {
                prelude_connection_destroy(cnx);
                return ret;
        }

        if ( cret < 0 ) {
                notify_dead(*new, cret, TRUE);

                if ( prelude_error_get_code(cret) == PRELUDE_ERROR_PROFILE )
                        return cret;
        }

        if ( pool->event_handler && pool->wanted_event & event )
                pool->event_handler(pool, event, cnx);

        return 0;
}




/*
 * Creates a list (boolean AND) of connections.
 */
static int create_connection_list(cnx_list_t **new, prelude_connection_pool_t *pool)
{
        *new = calloc(1, sizeof(**new));
        if ( ! *new )
                return prelude_error_from_errno(errno);

        (*new)->parent = pool;

        return 0;
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
static int parse_config_line(prelude_connection_pool_t *pool)
{
        int ret;
        cnx_t **cnx;
        cnx_list_t *clist = NULL;
        char *ptr, *cfgline = pool->connection_string;

        ret = create_connection_list(&pool->or_list, pool);
        if ( ret < 0 )
                return ret;

        clist = pool->or_list;
        cnx = &clist->and;

        while ( 1 ) {
                ptr = parse_config_string(&cfgline);

                /*
                 * If we meet end of line or "||",
                 * it means we just finished adding a AND list.
                 */
                if ( ! ptr || (ret = strcmp(ptr, "||") == 0) ) {

                        /*
                         * end of line ?
                         */
                        if ( ! ptr )
                                break;

                        /*
                         * we met the || operator, prepare a new list.
                         */
                        ret = create_connection_list(&clist->or, pool);
                        if ( ret < 0 )
                                return ret;

                        clist = clist->or;
                        cnx = &clist->and;
                        continue;
                }

                ret = strcmp(ptr, "&&");
                if ( ret == 0 )
                        continue;

                ret = new_connection_from_address(cnx, pool->client_profile, clist, ptr, pool->flags);
                if ( ret < 0 )
                        return ret;

                clist->total++;
                cnx = &(*cnx)->and;
        }

        return 0;
}




static cnx_t *search_cnx(prelude_connection_pool_t *pool, prelude_connection_t *cnx)
{
        cnx_t *c;
        cnx_list_t *clist;

        for ( clist = pool->or_list; clist != NULL; clist = clist->or ) {

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
        prelude_connection_pool_t *pool = data;

        prelude_connection_pool_broadcast(pool, msg);
        prelude_msg_destroy(msg);

        prelude_connection_pool_destroy(pool);
}




/**
 * prelude_connection_pool_broadcast:
 * @pool: Pointer to a #prelude_connection_pool_t object.
 * @msg: Pointer on a #prelude_msg_t object.
 *
 * Sends the message contained in @msg to all the connection in @pool.
 */
void prelude_connection_pool_broadcast(prelude_connection_pool_t *pool, prelude_msg_t *msg)
{
        prelude_return_if_fail(pool && msg);
        walk_manager_lists(pool, msg);
}




/**
 * prelude_connection_pool_broadcast_async:
 * @pool: Pointer to a #prelude_connection_pool_t object
 * @msg: Pointer on a #prelude_msg_t object.
 *
 * Sends the message contained in @msg to all connections
 * in @pool asynchronously. After the request is processed,
 * the @msg message will be freed.
 */
void prelude_connection_pool_broadcast_async(prelude_connection_pool_t *pool, prelude_msg_t *msg)
{
        prelude_return_if_fail(pool && msg);

        pool->refcount++;

        prelude_async_set_callback((prelude_async_object_t *) msg, &broadcast_async_cb);
        prelude_async_set_data((prelude_async_object_t *) msg, pool);
        prelude_async_add((prelude_async_object_t *) msg);
}



/**
 * prelude_connection_pool_init:
 * @pool: Pointer to a #prelude_connection_pool_t object.
 *
 * Initializes @pool. This means that connection associated
 * with @pool using prelude_connection_pool_set_connection_string()
 * will be established.
 *
 * Returns: 0 on success, a negative value on error.
 */
int prelude_connection_pool_init(prelude_connection_pool_t *pool)
{
        int ret;
        cnx_list_t *clist;
        char dirname[512], buf[512];

        prelude_return_val_if_fail(pool, prelude_error(PRELUDE_ERROR_ASSERTION));

        if ( ! pool->failover && (pool->flags & PRELUDE_CONNECTION_POOL_FLAGS_FAILOVER) ) {
                prelude_client_profile_get_backup_dirname(pool->client_profile, buf, sizeof(buf));
                snprintf(dirname, sizeof(dirname), "%s/global", buf);

                ret = prelude_failover_new(&pool->failover, dirname);
                if ( ret < 0 )
                        return ret;
        }

        if ( ! pool->connection_string_changed || ! pool->connection_string )
                return prelude_error(PRELUDE_ERROR_CONNECTION_STRING);

        pool->connection_string_changed = FALSE;
        connection_list_destroy(pool->or_list);

        pool->nfd = 0;
        pool->or_list = NULL;
        prelude_list_init(&pool->all_cnx);

        ret = parse_config_line(pool);
        if ( ret < 0 || ! pool->or_list )
                return ret;

        for ( clist = pool->or_list; clist != NULL; clist = clist->or ) {

                if ( clist->dead )
                        continue;

                if ( pool->failover ) {
                        ret = failover_flush(pool->failover, clist, NULL);
                        if ( ret == 0 )
                                break;
                }
        }

        if ( pool->global_event_handler )
                pool->global_event_handler(pool, 0);

        if ( ret < 0 )
                prelude_log(PRELUDE_LOG_WARN, "Can't contact configured Manager - Enabling failsafe mode.\n");

        if ( pool->wanted_event & PRELUDE_CONNECTION_POOL_EVENT_INPUT ) {
                prelude_timer_set_data(&pool->timer, pool);
                prelude_timer_set_expire(&pool->timer, 1);
                prelude_timer_set_callback(&pool->timer, check_for_data_cb);
                prelude_timer_init(&pool->timer);
        }

        return 0;
}



/**
 * prelude_connection_pool_new:
 * @ret: Pointer to an address where to store the created #prelude_connection_pool_t object.
 * @cp: The #prelude_client_profile_t to use for connection.
 * @permission: Permission the connection in this connection-pool will require.
 *
 * prelude_connection_pool_new() initializes a new Connection Manager object.
 *
 * Returns: 0 on success or a negative value if an error occured.
 */
int prelude_connection_pool_new(prelude_connection_pool_t **ret,
                                prelude_client_profile_t *cp,
                                prelude_connection_permission_t permission)
{
        prelude_connection_pool_t *new;

        prelude_return_val_if_fail(cp, prelude_error(PRELUDE_ERROR_ASSERTION));

        *ret = new = calloc(1, sizeof(*new));
        if ( ! new )
                return prelude_error_from_errno(errno);

        FD_ZERO(&new->fds);
        new->refcount = 1;
        new->client_profile = cp;
        new->permission = permission;
        new->connection_string_changed = FALSE;
        new->flags = PRELUDE_CONNECTION_POOL_FLAGS_FAILOVER;

        prelude_list_init(&new->all_cnx);
        prelude_list_init(&new->int_cnx);
        prelude_timer_init_list(&new->timer);

        return 0;
}



/**
 * prelude_connection_pool_destroy:
 * @pool: Pointer to a #prelude_connection_pool_t object.
 *
 * Destroys @pool and all connections handled.
 */
void prelude_connection_pool_destroy(prelude_connection_pool_t *pool)
{
        prelude_return_if_fail(pool);

        if ( --pool->refcount != 0 )
                return;

        prelude_timer_destroy(&pool->timer);

        if ( pool->connection_string )
                free(pool->connection_string);

        connection_list_destroy(pool->or_list);

        if ( pool->failover )
                prelude_failover_destroy(pool->failover);

        free(pool);
}



/**
 * prelude_connection_pool_add_connection:
 * @pool: Pointer to a #prelude_connection_pool_t object.
 * @cnx: Pointer to a #prelude_connection_t object to add to @pool.
 *
 * Adds @cnx to @pool set of connections.
 *
 * Returns: 0 on success, a negative value if an error occured.
 */
int prelude_connection_pool_add_connection(prelude_connection_pool_t *pool, prelude_connection_t *cnx)
{
        int ret;
        cnx_t **c;

        prelude_return_val_if_fail(pool && cnx, prelude_error(PRELUDE_ERROR_ASSERTION));

        if ( ! pool->or_list ) {
                ret = create_connection_list(&pool->or_list, pool);
                if ( ret < 0 )
                        return ret;
        }

        for ( c = &pool->or_list->and; (*c); c = &(*c)->and );

        ret = new_connection(c, pool->client_profile, pool->or_list, cnx, pool->flags);
        if ( ret < 0 )
                return ret;

        pool->or_list->total++;

        if ( (*c)->parent->dead == 0 && pool->failover ) {
                ret = failover_flush(pool->failover, (*c)->parent, NULL);
                if ( ret < 0 )
                        return ret;
        }

        return 0;
}



/**
 * prelude_connection_pool_set_global_event_handler:
 * @pool: Pointer to a #prelude_connection_pool_t object.
 * @wanted_events: Event the user want to be notified about.
 * @callback: User specific callback to call when an event is available.
 *
 * @callback will be called each time one of the event specified in
 * @wanted_events happen to @pool. However, contrary to
 * prelude_connection_pool_set_event_handler(), the callback will be called
 * only once per set of event.
 */
void prelude_connection_pool_set_global_event_handler(prelude_connection_pool_t *pool,
                                                      prelude_connection_pool_event_t wanted_events,
                                                      int (*callback)(prelude_connection_pool_t *pool,
                                                                      prelude_connection_pool_event_t events))
{
        prelude_return_if_fail(pool);

        pool->wanted_event = wanted_events;
        pool->global_event_handler = callback;
}



/**
 * prelude_connection_pool_set_event_handler:
 * @pool: Pointer to a #prelude_connection_pool_t object.
 * @wanted_events: Event the user want to be notified about.
 * @callback: User specific callback to call when an event is available.
 *
 * @callback will be called each time one of the event specified in
 * @wanted_events happens to @pool.
 */
void prelude_connection_pool_set_event_handler(prelude_connection_pool_t *pool,
                                               prelude_connection_pool_event_t wanted_events,
                                               int (*callback)(prelude_connection_pool_t *pool,
                                                               prelude_connection_pool_event_t events,
                                                               prelude_connection_t *cnx))
{
        prelude_return_if_fail(pool);

        pool->wanted_event = wanted_events;
        pool->event_handler = callback;
}



/**
 * prelude_connection_pool_get_connection_list:
 * @pool: Pointer to a #prelude_connection_pool_t object.
 *
 * Returns: The list of connections handled by @pool.
 */
prelude_list_t *prelude_connection_pool_get_connection_list(prelude_connection_pool_t *pool)
{
        prelude_return_val_if_fail(pool, NULL);
        return &pool->all_cnx;
}



/**
 * prelude_connection_pool_set_connection_dead:
 * @pool: Pointer to a #prelude_connection_pool_t object.
 * @cnx: Pointer to a #prelude_connection_t object used within @pool.
 *
 * Notifies @pool that the connection identified by @cnx is dead.
 *
 * Usually, this function should not be used since @pool is
 * self sufficient, and handles connections issues internally. However,
 * it is sometime useful when the user has several mechanisms using the
 * connection, and that its own mechanism detects a connection problem
 * before @pool notice.
 *
 * Returns: 0 on success, a negative value if an error occured.
 */
int prelude_connection_pool_set_connection_dead(prelude_connection_pool_t *pool, prelude_connection_t *cnx)
{
        cnx_t *c;

        prelude_return_val_if_fail(pool && cnx, prelude_error(PRELUDE_ERROR_ASSERTION));

        c = search_cnx(pool, cnx);
        if ( ! c )
                return -1;

        if ( c->is_dead )
                return 0;

        c->is_dead = TRUE;
        c->parent->dead++;
        init_cnx_timer(c);

        return 0;
}



/**
 * prelude_connection_pool_set_connection_alive:
 * @pool: Pointer to a #prelude_connection_pool_t object.
 * @cnx: Pointer to a #prelude_connection_t object used within @pool.
 *
 * Notifies @pool that the connection identified by @cnx went back alive.
 *
 * Usually, this function should not be used since @pool is
 * self sufficient, and handles connection issues internally. However,
 * it is sometime useful when the user has several mechanisms using the
 * connection, and that its own mechanism detects a connection problem
 * before @pool notice.
 *
 * Returns: 0 on success, a negative value if an error occured.
 */
int prelude_connection_pool_set_connection_alive(prelude_connection_pool_t *pool, prelude_connection_t *cnx)
{
        int ret;
        cnx_t *c;

        prelude_return_val_if_fail(pool && cnx, prelude_error(PRELUDE_ERROR_ASSERTION));

        c = search_cnx(pool, cnx);
        if ( ! c )
                return -1;

        if ( c->parent->dead == 0 )
                return 0;

        c->parent->dead--;
        c->is_dead = FALSE;

        ret = failover_flush(c->failover, NULL, c);
        if ( ret < 0 )
                return ret;

        if ( c->parent->dead == 0 && pool->failover ) {
                ret = failover_flush(pool->failover, c->parent, NULL);
                if ( ret < 0 )
                        return ret;
        }

        return 0;
}



/**
 * prelude_connection_pool_set_connection_string:
 * @pool: Pointer to a #prelude_connection_pool_t object.
 * @cfgstr: Connection string.
 *
 * Sets the connection string for @pool. The connection string should be
 * in the form of : "address". Special operand like || (OR) and && (AND),
 * are also accepted: "address && address".
 *
 * Where && means that alert sent using @pool will go to both configured
 * addresses, and || means that if the left address fails, the right address
 * will be used.
 *
 * prelude_connection_pool_init() should be used to initiates the connection.
 *
 * Returns: 0 on success, a negative value if an error occured.
 */
int prelude_connection_pool_set_connection_string(prelude_connection_pool_t *pool, const char *cfgstr)
{
        char *new;

        prelude_return_val_if_fail(pool && cfgstr, prelude_error(PRELUDE_ERROR_ASSERTION));

        new = strdup(cfgstr);
        if ( ! new )
                return prelude_error_from_errno(errno);

        if ( pool->connection_string )
                free(pool->connection_string);

        pool->connection_string = new;
        pool->connection_string_changed = TRUE;

        return 0;
}



/**
 * prelude_connection_pool_get_connection_string:
 * @pool: Pointer to a #prelude_connection_pool_t object.
 *
 * Used to query the connection string used by @pool.
 *
 * Returns: The connection string.
 */
const char *prelude_connection_pool_get_connection_string(prelude_connection_pool_t *pool)
{
        prelude_return_val_if_fail(pool, NULL);
        return pool->connection_string;
}



/**
 * prelude_connection_pool_set_flags:
 * @pool: Pointer to a #prelude_connection_pool_t object.
 * @flags: Flags to use for @pool.
 *
 * Sets @flags within @pools.
 */
void prelude_connection_pool_set_flags(prelude_connection_pool_t *pool, prelude_connection_pool_flags_t flags)
{
        prelude_return_if_fail(pool);
        pool->flags = flags;
}



void prelude_connection_pool_set_required_permission(prelude_connection_pool_t *pool, prelude_connection_permission_t req_perm)
{
        prelude_return_if_fail(pool);
        pool->permission = req_perm;
}




/**
 * prelude_connection_pool_get_flags:
 * @pool: Pointer to a #prelude_connection_pool_t object.
 *
 * Returns: the #prelude_connection_pool_flags_t used in @pool.
 */
prelude_connection_pool_flags_t prelude_connection_pool_get_flags(prelude_connection_pool_t *pool)
{
        prelude_return_val_if_fail(pool, prelude_error(PRELUDE_ERROR_ASSERTION));
        return pool->flags;
}



static int connection_pool_check_event(prelude_connection_pool_t *pool, int timeout,
                                       int (*event_cb)(prelude_connection_pool_t *pool,
                                                       prelude_connection_pool_event_t event,
                                                       prelude_connection_t *cnx, void *extra),
                                       void *extra, prelude_connection_t **conn, prelude_msg_t **outmsg)
{
        cnx_t *cnx;
        fd_set rfds;
        int ret, i = 0, fd;
        prelude_list_t *tmp;
        struct timeval tv, *tvptr = NULL;
        prelude_connection_pool_event_t global_event = 0;

        if ( timeout >= 0 ) {
                tvptr = &tv;
                tv.tv_sec = timeout;
                tv.tv_usec = 0;
        }

        rfds = pool->fds;

        ret = select(pool->nfd, &rfds, NULL, NULL, tvptr);
        if ( ret < 0 )
                return prelude_error_from_errno(errno);

        else if ( ret == 0 )
                return 0;

        prelude_list_for_each(&pool->int_cnx, tmp) {
                cnx = prelude_list_entry(tmp, cnx_t, list);

                if ( ! prelude_connection_is_alive(cnx->cnx) )
                        continue;

                fd = prelude_io_get_fd(prelude_connection_get_fd(cnx->cnx));
                assert(fd < FD_SETSIZE);

                ret = FD_ISSET(fd, &rfds);
                if ( ! ret )
                        continue;

                global_event |= PRELUDE_CONNECTION_POOL_EVENT_INPUT;

                i++;
                if ( conn ) {
                        *conn = cnx->cnx;

                        if ( outmsg ) {
                                do {
                                        ret = prelude_connection_recv(cnx->cnx, outmsg);
                                } while ( ret < 0 && prelude_error_get_code(ret) == PRELUDE_ERROR_EAGAIN );

                                if ( ret < 0 ) {
                                        global_event |= PRELUDE_CONNECTION_POOL_EVENT_DEAD;
                                        notify_dead(cnx, ret, FALSE);

                                        i--;
                                        continue;
                                }
                        }

                        break;
                }

                else if ( event_cb ) {
                        ret = event_cb(pool, PRELUDE_CONNECTION_POOL_EVENT_INPUT, cnx->cnx, extra);
                        if ( ret < 0 ) {
                                global_event |= PRELUDE_CONNECTION_POOL_EVENT_DEAD;
                                notify_dead(cnx, ret, FALSE);
                        }
                }

                else if ( pool->event_handler ) {
                        ret = pool->event_handler(pool, PRELUDE_CONNECTION_POOL_EVENT_INPUT, cnx->cnx);
                        if ( ret < 0 || ! prelude_connection_is_alive(cnx->cnx) ) {
                                global_event |= PRELUDE_CONNECTION_POOL_EVENT_DEAD;
                                notify_dead(cnx, ret, FALSE);
                        }
                }
        }

        if ( global_event & pool->wanted_event && pool->global_event_handler )
                pool->global_event_handler(pool, global_event);

        if ( pool->connection_string_changed )
                prelude_connection_pool_init(pool);

        return i;
}



/**
 * prelude_connection_pool_check_event:
 * @pool: Pointer to a #prelude_connection_pool_t object.
 * @timeout: Time to wait for an event.
 * @event_cb: User provided callback function to call on received events.
 * @extra: Pointer to user specific data provided to @event_cb.
 *
 * This function queries the set of connections available in @pool to see if
 * events are waiting to be handled. If timeout is zero, then this function
 * will return immediatly in case there is no event to be handled.
 *
 * If timeout is -1, this function won't return until an event is available.
 * Otherwise this function will return if there is no event after the specified
 * number of second.
 *
 * For each event, @event_cb is called with the concerned @pool, the provided
 * @extra data, and the @cnx where an event has occured.
 *
 * Returns: The number of handled events, or a negative value if an error occured.
 */
int prelude_connection_pool_check_event(prelude_connection_pool_t *pool, int timeout,
                                        int (*event_cb)(prelude_connection_pool_t *pool,
                                                        prelude_connection_pool_event_t event,
                                                        prelude_connection_t *cnx, void *extra), void *extra)
{
        prelude_return_val_if_fail(pool, prelude_error(PRELUDE_ERROR_ASSERTION));
        return connection_pool_check_event(pool, timeout, event_cb, extra, NULL, NULL);
}



/**
 * prelude_connection_pool_recv:
 * @pool: Pointer to a #prelude_connection_pool_t object.
 * @timeout: Time to wait for an event.
 * @outconn: Pointer where the connection where an event happened should be stored.
 * @outmsg: Pointer where the next message that will be read should be stored.
 *
 * This function queries the set of connections available in @pool to see if
 * events are waiting to be handled. If timeout is zero, then this function
 * will return immediatly in case there is no event to be handled.
 *
 * If timeout is -1, this function won't return until an event is available.
 * Otherwise this function will return if there is no event after the specified
 * number of second.
 *
 * If an event is available, it will be read and store the #prelude_connection_t
 * object in the @outconn pointer. If @outmsg was specified, the message will be
 * read and stored in there.
 *
 * Returns: The number of handled events (0 or 1) or a negative value if an error occured.
 */
int prelude_connection_pool_recv(prelude_connection_pool_t *pool, int timeout, prelude_connection_t **outconn, prelude_msg_t **outmsg)
{
        prelude_return_val_if_fail(pool, prelude_error(PRELUDE_ERROR_ASSERTION));
        return connection_pool_check_event(pool, timeout, NULL, NULL, outconn, outmsg);
}




/**
 * prelude_connection_pool_set_data:
 * @pool: Pointer to a #prelude_connection_pool_t object.
 * @data: Pointer to user specific data.
 *
 * The user might use this function to associate data with @pool.
 * The data associated might be retrieved using prelude_connection_pool_get_data().
 */
void prelude_connection_pool_set_data(prelude_connection_pool_t *pool, void *data)
{
        prelude_return_if_fail(pool);
        pool->data = data;
}



/**
 * prelude_connection_pool_get_data:
 * @pool: Pointer to a #prelude_connection_pool_t object.
 *
 * The user might use this function to query data associated with
 * @pool using prelude_connection_pool_set_data().
 *
 * Returns: the user data associated to @pool.
 */
void *prelude_connection_pool_get_data(prelude_connection_pool_t *pool)
{
        prelude_return_val_if_fail(pool, NULL);
        return pool->data;
}
