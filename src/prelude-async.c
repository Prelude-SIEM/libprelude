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
#include <sys/time.h>
#include <inttypes.h>
#include <pthread.h>
#include <errno.h>

#include "thread.h"
#include "prelude-list.h"
#include "timer.h"
#include "common.h"
#include "prelude-io.h"
#include "prelude-message.h"
#include "prelude-async.h"


static LIST_HEAD(joblist);

static pthread_t thread;
static pthread_cond_t cond;
static pthread_mutex_t mutex;


int (*prelude_async_lock)(prelude_async_object_t *obj);
int (*prelude_async_unlock)(prelude_async_object_t *obj);



static double get_elapsed_time(struct timeval *start) 
{
        struct timeval tv;
        double current, s;
        
        gettimeofday(&tv, NULL);
        
        current = (double) tv.tv_sec + (double) (tv.tv_usec * 1e-6);
        s = (double) start->tv_sec + (double) (start->tv_usec * 1e-6);

        return current - s;
}



static void wait_timer_and_data(void) 
{
        int ret;
        double elapsed;
        struct timespec ts;
        struct timeval now;
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
                while ( list_empty(&joblist) && ret != ETIMEDOUT ) {
                        ret = pthread_cond_timedwait(&cond, &mutex, &ts);
                }
                pthread_mutex_unlock(&mutex);
                
                /*
                 * Data is available for processing, but we also want to check
                 * the average time we spent waiting on the condition. (which may be
                 * > 1 second if the condition was signaled several time).
                 */
                if ( ret != ETIMEDOUT ) {
                        elapsed = get_elapsed_time(&last_timer_wake_up);
                        if ( elapsed >= 1 ) {
                                gettimeofday(&last_timer_wake_up, NULL);
                                prelude_wake_up_timer();
                        }
                        return;
                }

                else {
                        gettimeofday(&last_timer_wake_up, NULL);
                        prelude_wake_up_timer();
                }
        }
}



static void *async_thread(void *arg) 
{
        prelude_async_object_t *obj;
        struct list_head *tmp, *next;
        
        
        while ( 1 ) {
                wait_timer_and_data();

                pthread_mutex_lock(&mutex);
                next = ( list_empty(&joblist) ) ? NULL : joblist.next;
                pthread_mutex_unlock(&mutex);
                
                while ( next ) {

                        tmp = next;
                        
                        pthread_mutex_lock(&mutex);
                        next = ( tmp->next != &joblist ) ? tmp->next : NULL;
                        pthread_mutex_unlock(&mutex);
                        
                        obj = prelude_list_get_object(tmp, prelude_async_object_t);
                        prelude_async_del(obj);
                        obj->func(obj, obj->data);                     
                }
        }
}



static int do_nothing(void) 
{
        return 0;
}



static int real_prelude_async_lock(prelude_async_object_t *obj) 
{
        return pthread_mutex_lock(&obj->mutex);
}


static int real_prelude_async_unlock(prelude_async_object_t *obj) 
{
        return pthread_mutex_unlock(&obj->mutex);
}



static void prelude_async_exit(void)  
{
        log(LOG_INFO, "Flushing remaining messages.\n");
        
        pthread_cancel(thread);
        pthread_cond_destroy(&cond);
        pthread_mutex_destroy(&mutex);
}



static int thread_async_init(void) 
{
        int ret;
        
        prelude_async_lock = real_prelude_async_lock;
        prelude_async_unlock = real_prelude_async_unlock;
        
        pthread_mutex_init(&mutex, NULL);
        pthread_cond_init(&cond, NULL);

        ret = pthread_create(&thread, NULL, async_thread, NULL);
        if ( ret < 0 ) {
                pthread_cond_destroy(&cond);
                pthread_mutex_destroy(&mutex);
                return -1;
        }

        return atexit(prelude_async_exit);
}



#if 0
static int async_init(void) 
{
        prelude_async_lock = noop_prelude_async_lock;
        prelude_async_unlock = noop_prelude_async_unlock;

        return 0;
}
#endif




/**
 * prelude_async_init:
 * @use_thread: boolean.
 *
 * Initialize the asynchronous subsystem.
 * Use a separate thread for asynchronous processing if @use_thread
 * is TRUE. Otherwise, the program have to call itself FIXME() from
 * the main program loop.
 *
 * Returns: O on success, -1 if an error occured.
 */
int prelude_async_init(int use_thread) 
{
        int ret = 0;

        if ( use_thread )
                ret = thread_async_init();
        else
#if 0
                ret = async_init();
#endif
        return ret;
}



/**
 * prelude_async_add:
 * @obj: Pointer on an asynchronous object.
 *
 * Add an asynchronous object to the queue.
 */
void prelude_async_add(prelude_async_object_t *obj) 
{
        prelude_async_lock(obj);
        prelude_list_add_tail((prelude_linked_object_t *)obj, &joblist);
        prelude_async_unlock(obj);
}



/**
 * prelude_async_del:
 * @obj: Pointer on an asynchronous object.
 *
 * Remove an asynchronous object from the queue.
 */
void prelude_async_del(prelude_async_object_t *obj) 
{
        prelude_async_lock(obj);
        prelude_list_del((prelude_linked_object_t *)obj);
        prelude_async_unlock(obj);
}






