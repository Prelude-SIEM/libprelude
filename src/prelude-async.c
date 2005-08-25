/*****
*
* Copyright (C) 2001, 2002, 2003, 2004, 2005 PreludeIDS Technologies. All Rights Reserved.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <assert.h>

#include "prelude-thread.h"
#include "prelude-list.h"
#include "prelude-inttypes.h"
#include "prelude-linked-object.h"
#include "prelude-timer.h"
#include "prelude-log.h"
#include "prelude-io.h"
#include "prelude-async.h"



/*
 * On POSIX systems where clock_gettime() is available, the symbol
 * _POSIX_TIMERS should be defined to a value greater than 0. 
 * 
 * However, some architecture (example True64), define it as:
 * #define _POSIX_TIMERS
 *
 * This explain the - 0 hack, since we need to test for the explicit
 * case where _POSIX_TIMERS is defined to a value higher than 0.
 *
 * If pthread_condattr_setclock and _POSIX_MONOTONIC_CLOCK are available,
 * CLOCK_MONOTONIC will be used. This avoid possible race problem when
 * calling pthread_cond_timedwait() if the system time is modified.
 *
 * If CLOCK_MONOTONIC is not available, revert to the standard CLOCK_REALTIME
 * way.
 *
 * If neither of the above are avaible, use gettimeofday().
 */
#if _POSIX_TIMERS - 0 > 0
# if defined(HAVE_PTHREAD_CONDATTR_SETCLOCK) && defined(_POSIX_MONOTONIC_CLOCK) && (_POSIX_MONOTONIC_CLOCK - 0 >= 0)
#  define COND_CLOCK_TYPE CLOCK_MONOTONIC
# else
#  define COND_CLOCK_TYPE CLOCK_REALTIME
# endif
#endif


static PRELUDE_LIST(joblist);


static prelude_async_flags_t async_flags = 0;
static prelude_bool_t stop_processing = FALSE;


static pthread_t thread;
static pthread_cond_t cond;
static pthread_mutex_t mutex;

static volatile sig_atomic_t is_initialized = FALSE;




static prelude_bool_t timer_need_wake_up(struct timespec *now, struct timespec *start) 
{
        time_t diff = now->tv_sec - start->tv_sec;
        
        if ( diff > 1 || (diff == 1 && now->tv_nsec >= start->tv_nsec) )
                return TRUE;

        return FALSE;
}


static void get_time(struct timespec *ts)
{
#if _POSIX_TIMERS - 0 > 0
        int ret;
        
        ret = clock_gettime(COND_CLOCK_TYPE, ts);
        if ( ret < 0 )
                prelude_log(PRELUDE_LOG_ERR, "clock_gettime: %s.\n", strerror(errno));

#else
        struct timeval now;

        gettimeofday(&now, NULL);
        
        ts->tv_sec = now.tv_sec;
        ts->tv_nsec = now.tv_usec * 1000;
#endif
}



static void wait_timer_and_data(void) 
{
        int ret;
        struct timespec ts;
        prelude_async_flags_t old_async_flags;
        prelude_bool_t no_job_available = TRUE;
        static struct timespec last_wake_up = { 0, 0 };
        
        while ( no_job_available ) {
                ret = 0;
                
                pthread_mutex_lock(&mutex);
                old_async_flags = async_flags;

                /*
                 * Setup the condition timer to one second.
                 */
                get_time(&ts);
                ts.tv_sec++;

                while ( (no_job_available = prelude_list_is_empty(&joblist)) &&
                        ! stop_processing && async_flags == old_async_flags && ret != ETIMEDOUT ) {
                        
                        ret = pthread_cond_timedwait(&cond, &mutex, &ts);
                }
                
                if ( no_job_available && stop_processing ) {
                        pthread_mutex_unlock(&mutex);
                        pthread_exit(NULL);
                }
                
                pthread_mutex_unlock(&mutex);

                get_time(&ts);
                if ( ret == ETIMEDOUT || timer_need_wake_up(&ts, &last_wake_up) ) {
                        prelude_timer_wake_up();
                        last_wake_up.tv_sec = ts.tv_sec;
                        last_wake_up.tv_nsec = ts.tv_nsec;
                }
        }
}




static void wait_data(void) 
{
        prelude_async_flags_t old_async_flags;
        
        pthread_mutex_lock(&mutex);
        old_async_flags = async_flags;
        
        while ( prelude_list_is_empty(&joblist) && ! stop_processing && async_flags == old_async_flags ) 
                pthread_cond_wait(&cond, &mutex);

        if ( prelude_list_is_empty(&joblist) && stop_processing ) {
                pthread_mutex_unlock(&mutex);
                pthread_exit(NULL);
        }

        pthread_mutex_unlock(&mutex);
}



static prelude_async_object_t *get_next_job(void)
{
        prelude_list_t *tmp;
        prelude_async_object_t *obj = NULL;

        pthread_mutex_lock(&mutex);
        
        prelude_list_for_each(&joblist, tmp) {
                obj = prelude_linked_object_get_object(tmp);
                prelude_linked_object_del((prelude_linked_object_t *) obj);
                break;
        }
        
        pthread_mutex_unlock(&mutex);

        return obj;
}



static void *async_thread(void *arg) 
{
        int ret;
        sigset_t set;
        prelude_async_object_t *obj;
        
        ret = sigfillset(&set);
        if ( ret < 0 ) {
                prelude_log(PRELUDE_LOG_ERR, "sigfillset returned an error.\n");
                return NULL;
        }
        
        ret = pthread_sigmask(SIG_BLOCK, &set, NULL);
        if ( ret < 0 ) {
                prelude_log(PRELUDE_LOG_ERR, "pthread_sigmask returned an error.\n");
                return NULL;
        }

        while ( 1 ) {
                
                if ( async_flags & PRELUDE_ASYNC_FLAGS_TIMER )
                        wait_timer_and_data();
                else
                        wait_data();
                
                while ( (obj = get_next_job()) )
                        obj->_async_func(obj, obj->_async_data);
        }
}




static void async_exit(void)  
{
        prelude_log(PRELUDE_LOG_INFO, "Waiting for asynchronous operation to complete.\n");
        prelude_async_exit();
}



#ifdef HAVE_PTHREAD_ATFORK

static void prepare_fork_cb(void)
{
        pthread_mutex_lock(&mutex);
}



static void parent_fork_cb(void)
{
        pthread_mutex_unlock(&mutex);
}



static void child_fork_cb(void)
{
        is_initialized = FALSE;
        prelude_list_init(&joblist);
}

#endif



static int do_init_async(void)
{
        int ret;
        pthread_condattr_t attr;

        ret = pthread_condattr_init(&attr);
        if ( ret != 0 ) {
                prelude_log(PRELUDE_LOG_ERR, "error initializing condition attribute: %s.\n", strerror(ret));
                return ret;
        }
        
#if defined(HAVE_PTHREAD_CONDATTR_SETCLOCK) && _POSIX_TIMERS - 0 > 0
        ret = pthread_condattr_setclock(&attr, COND_CLOCK_TYPE);
        if ( ret != 0 ) {
                prelude_log(PRELUDE_LOG_ERR, "error setting condition clock attribute: %s.\n", strerror(ret));
                return ret;
        }
#endif
        
        ret = pthread_cond_init(&cond, &attr);
        if ( ret != 0 ) {
                prelude_log(PRELUDE_LOG_ERR, "error creating condition: %s.\n", strerror(ret));
                return ret;
        }

        ret = pthread_mutex_init(&mutex, NULL);
        if ( ret != 0 ) {
                prelude_log(PRELUDE_LOG_ERR, "error creating mutex: %s.\n", strerror(ret));
                return ret;
        }

                  
#ifdef HAVE_PTHREAD_ATFORK
        {
                static volatile sig_atomic_t fork_handler_registered = FALSE;
                
                if ( ! fork_handler_registered ) {
                        fork_handler_registered = TRUE;
                        pthread_atfork(prepare_fork_cb, parent_fork_cb, child_fork_cb);
                }
        }
#endif
        
        ret = pthread_create(&thread, NULL, async_thread, NULL);
        if ( ret != 0 ) {
                prelude_log(PRELUDE_LOG_ERR, "error creating asynchronous thread: %s.\n", strerror(ret));
                return ret;
        }
        
        return atexit(async_exit);
}



/**
 * prelude_async_set_flags:
 * @flags: flags you want to set
 * 
 * Sets flags to the asynchronous subsystem.
 *
 */
void prelude_async_set_flags(prelude_async_flags_t flags) 
{
        pthread_mutex_lock(&mutex);
        
        async_flags = flags;
        pthread_cond_signal(&cond);

        pthread_mutex_unlock(&mutex);
}



/**
 * prelude_async_get_flags:
 * 
 * Retrieves flags from the asynchronous subsystem
 *
 * Returns: asynchronous flags
 */
prelude_async_flags_t prelude_async_get_flags(void) 
{
        return async_flags;
}



/**
 * prelude_async_init:
 *
 * Initialize the asynchronous subsystem.
 *
 * Returns: 0 on success, -1 if an error occured.
 */
int prelude_async_init(void) 
{
        if ( ! is_initialized ) {
                assert(_prelude_thread_in_use() == TRUE);
                
                is_initialized = TRUE;                
                return do_init_async();
        }
        
        return 0;
}



/**
 * prelude_async_add:
 * @obj: Pointer to a #prelude_async_t object.
 *
 * Adds @obj to the asynchronous processing list.
 */
void prelude_async_add(prelude_async_object_t *obj) 
{        
        pthread_mutex_lock(&mutex);
        
        prelude_linked_object_add_tail(&joblist, (prelude_linked_object_t *) obj);        
        pthread_cond_signal(&cond);

        pthread_mutex_unlock(&mutex);
}



/**
 * prelude_async_del:
 * @obj: Pointer to a #prelude_async_t object.
 *
 * Deletes @obj from the asynchronous processing list.
 */
void prelude_async_del(prelude_async_object_t *obj) 
{
        pthread_mutex_lock(&mutex);
        prelude_linked_object_del((prelude_linked_object_t *) obj);
        pthread_mutex_unlock(&mutex);
}




void prelude_async_exit(void)
{        
        pthread_mutex_lock(&mutex);

        stop_processing = TRUE;
        pthread_cond_signal(&cond);
        
        pthread_mutex_unlock(&mutex);

        pthread_join(thread, NULL);
        pthread_cond_destroy(&cond);
        pthread_mutex_destroy(&mutex);
}
