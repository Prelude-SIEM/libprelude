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

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <inttypes.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>

#include "prelude-linked-object.h"
#include "timer.h"
#include "prelude-log.h"
#include "prelude-io.h"
#include "prelude-message.h"
#include "prelude-async.h"
#include "threads.h"



static LIST_HEAD(joblist);


static int async_flags = PRELUDE_ASYNC_TIMER;


static pthread_t thread;
static pthread_cond_t cond;
static pthread_mutex_t mutex;
static int stop_processing = 0;




static double get_elapsed_time(struct timeval *now, struct timeval *start) 
{
        double current, s;
                
        current = (double) now->tv_sec + (double) (now->tv_usec * 1e-6);
        s = (double) start->tv_sec + (double) (start->tv_usec * 1e-6);

        return current - s;
}



static void wait_timer_and_data(void) 
{
        int ret;
        double elapsed;
        struct timeval now;
        struct timespec ts;
        static struct timeval last_timer_wake_up;

        while ( 1 ) {
                ret = 0;
                
                /*
                 * Setup the condition timer to one second.
                 */
                gettimeofday(&now, NULL);
                ts.tv_sec = now.tv_sec + 1;
                ts.tv_nsec = now.tv_usec * 1000;

                pthread_mutex_lock(&mutex);

                while ( list_empty(&joblist) && ret != ETIMEDOUT && ! stop_processing ) {
                        ret = pthread_cond_timedwait(&cond, &mutex, &ts);
                }
                
                if ( list_empty(&joblist) && stop_processing ) {
                        pthread_mutex_unlock(&mutex);
                        pthread_exit(NULL);
                }
                
                pthread_mutex_unlock(&mutex);
                
                gettimeofday(&now, NULL);
                
                elapsed = get_elapsed_time(&now, &last_timer_wake_up);
                if ( elapsed >= 1 ) {
                        prelude_wake_up_timer();
                        last_timer_wake_up.tv_sec = now.tv_sec;
                        last_timer_wake_up.tv_usec = now.tv_usec;
                }

                /*
                 * if data available.
                 */
                if ( ret != ETIMEDOUT )
                        return;
        }
}




static void wait_data(void) 
{        
        pthread_mutex_lock(&mutex);
        
        while ( list_empty(&joblist) && ! stop_processing ) 
                pthread_cond_wait(&cond, &mutex);

        if ( list_empty(&joblist) && stop_processing ) {
                pthread_mutex_unlock(&mutex);
                pthread_exit(NULL);
        }

        pthread_mutex_unlock(&mutex);
}




static void *async_thread(void *arg) 
{
        int ret;
        sigset_t set;
        prelude_async_object_t *obj;
        struct list_head *tmp, *next;

        ret = sigfillset(&set);
        if ( ret < 0 ) {
                log(LOG_ERR, "sigfillset returned an error.\n");
                return NULL;
        }
        
        ret = pthread_sigmask(SIG_BLOCK, &set, NULL);
        if ( ret < 0 ) {
                log(LOG_ERR, "pthread_sigmask returned an error.\n");
                return NULL;
        }

        while ( 1 ) {

                if ( async_flags & PRELUDE_ASYNC_TIMER )
                        wait_timer_and_data();
                else
                        wait_data();
                
                pthread_mutex_lock(&mutex);
                next = ( list_empty(&joblist) ) ? NULL : joblist.next;
                pthread_mutex_unlock(&mutex);
                
                while ( next ) {

                        tmp = next;
                        
                        pthread_mutex_lock(&mutex);
                        next = ( tmp->next != &joblist ) ? tmp->next : NULL;
                        pthread_mutex_unlock(&mutex);
                        
                        obj = prelude_linked_object_get_object(tmp, prelude_async_object_t);
                        prelude_async_del(obj);
                        obj->func(obj, obj->data);                     
                }
        }
}




static void prelude_async_exit(void)  
{
        pthread_mutex_lock(&mutex);
        
        stop_processing = 1;
        pthread_cond_signal(&cond);

        pthread_mutex_unlock(&mutex);

        log(LOG_INFO, "Waiting for asynchronous operation to finish.\n");
        
        pthread_join(thread, NULL);
        
        pthread_cond_destroy(&cond);
        pthread_mutex_destroy(&mutex);
}




void prelude_async_set_flags(int flags) 
{
        async_flags = flags;
}



int prelude_async_get_flags(void) 
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
        int ret;
        
        pthread_mutex_init(&mutex, NULL);
        pthread_cond_init(&cond, NULL);
        
        ret = pthread_create(&thread, NULL, async_thread, NULL);
        if ( ret < 0 ) {
                pthread_cond_destroy(&cond);
                pthread_mutex_destroy(&mutex);
        }

        return atexit(prelude_async_exit);
}



/**
 * prelude_async_add:
 * @obj: Pointer to a #prelude_async_t object.
 *
 * Add @obj to the asynchronous processing list.
 */
void prelude_async_add(prelude_async_object_t *obj) 
{
        pthread_mutex_lock(&mutex);
        prelude_linked_object_add_tail((prelude_linked_object_t *)obj, &joblist);
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&mutex);
}




/**
 * prelude_async_del:
 * @obj: Pointer to a #prelude_async_t object.
 *
 * Delete @obj from the asynchronous processing list.
 */
void prelude_async_del(prelude_async_object_t *obj) 
{
        pthread_mutex_lock(&mutex);
        prelude_linked_object_del((prelude_linked_object_t *)obj);
        pthread_mutex_unlock(&mutex);
}



