/*****
*
* Copyright (C) 1999 - 2001 Yoann Vandoorselaere <yoann@mandrakesoft.com>
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
#include <assert.h>
#include <pthread.h>

#include "timer.h"
#include "common.h"


static LIST_HEAD(timer_list);
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;



/*
 * Return the time elapsed by a timer 'timer' from now,
 * to the time it was created / reset.
 */
static double time_elapsed(prelude_timer_t *timer) 
{
        struct timeval tv;
        double current, start;
        
        gettimeofday(&tv, NULL);
        current = (double) tv.tv_sec + (double) (tv.tv_usec * 1e-6);
        start = (double)timer->start.tv_sec + (double) (timer->start.tv_usec * 1e-6); 

        return current - start;
}



/*
 * If timer 'timer' need to be waked up (it elapsed >= time
 * for it to expire), call it's callback function, with it's
 * registered argument.
 *
 * All expired timer should be destroyed.
 */
static void wake_up_if_needed(prelude_timer_t *timer) 
{
        assert(timer->start.tv_sec != -1);

        if ( time_elapsed(timer) >= timer_expire(timer) ) {
                timer->start.tv_sec = -1;
                timer_func(timer)(timer_data(timer));
        }
}




/*
 * Walk the list of timer,
 * call the wake_up_if_need_function on each timer.
 */
static int walk_and_wake_up_timer(void) 
{
        prelude_timer_t *timer;
        struct list_head *tmp, *next;
        int usage = 0;
        
        pthread_mutex_lock(&mutex);
        next = ( list_empty(&timer_list) ) ? NULL : timer_list.next;
        pthread_mutex_unlock(&mutex);
        
        while ( next ) {
                tmp = next;
                
                pthread_mutex_lock(&mutex);
                next = ( tmp->next != &timer_list ) ? tmp->next : NULL;
                pthread_mutex_unlock(&mutex);
                
                timer = list_entry(tmp, prelude_timer_t, list);
                wake_up_if_needed(timer);
                usage++;
        }
        
        return usage;
}



/**
 * timer_init:
 * @timer: timer to initialize.
 *
 * Initialize a timer (add it to the timer list).
 */
void timer_init(prelude_timer_t *timer)
{
        gettimeofday(&timer->start, NULL);

        pthread_mutex_lock(&mutex);
        list_add(&timer->list, &timer_list);
        pthread_mutex_unlock(&mutex);
}



/**
 * timer_reset:
 * @timer: the timer to reset.
 *
 * Reset timer 'timer', as if it was just started.
 */
void timer_reset(prelude_timer_t *timer) 
{
        gettimeofday(&timer->start, NULL);
}



/**
 * timer_destroy:
 * @timer: the timer to destroy.
 * 
 * Destroy the timer 'timer',
 * this remove it from the active timer list.
 */
void timer_destroy(prelude_timer_t *timer) 
{
        pthread_mutex_lock(&mutex);
        list_del(&timer->list);
        pthread_mutex_unlock(&mutex);
}



/**
 * timer_elapsed:
 * @timer: the timer to get elapsed time from.
 * @tv: a #timeval structure to store the result in.
 *
 * Give the time elapsed by timer 'timer' from the last time
 * it was reset'd or from the time it was started.
 * The result is stored in a #timeval structure given as argument.
 */
void timer_elapsed(prelude_timer_t *timer, struct timeval *tv) 
{
        struct timeval current;

        gettimeofday(&current, NULL);
        
        tv->tv_sec  = current.tv_sec - timer->start.tv_sec;
        tv->tv_usec = current.tv_usec - timer->start.tv_usec;
}




/**
 * prelude_wake_up_timer:
 *
 * Wake up timer that need it.
 * This function should be called every second to work properly.
 */
void prelude_wake_up_timer(void) 
{
#if 0
        struct timeval tv, end; 
        int usage = 0;
                
        gettimeofday(&tv, NULL);
        usage = walk_and_wake_up_timer();
        gettimeofday(&end, NULL);
        
        log(LOG_INFO, "Debug: %u timer in use, wake up took %fs\n", usage,
            (end.tv_sec + (double) (end.tv_usec * 1e-6)) -
            (tv.tv_sec  + (double) (tv.tv_usec * 1e-6)));
#endif
        
        walk_and_wake_up_timer();
}





