/*****
*
* Copyright (C) 1999,2000 Yoann Vandoorselaere <yoann@mandrakesoft.com>
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

#include "timer.h"
#include "common.h"


static LIST_HEAD(timer_list);
static prelude_timer_t *current_timer;


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
                current_timer = timer;
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
        struct list_head *tmp, *bkp;
        int usage = 0;

        for ( tmp = timer_list.next; tmp != &timer_list; tmp = bkp ) {
                usage++;
                
                timer = list_entry(tmp, prelude_timer_t, list);
                bkp = tmp->next;

                wake_up_if_needed(timer);
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
        list_add(&timer->list, &timer_list);
}



/**
 * timer_destroy_current:
 *
 * Destroy currently expiring timer,
 * this is only to be called from a timer expire callback.
 */
void timer_destroy_current(void) 
{
        timer_destroy(current_timer);
}



/**
 * timer_reset_current:
 *
 * Reset currently expiring timer,
 * this is only to be called from a timer expire callback.
 */
void timer_reset_current(void) 
{
        timer_reset(current_timer);
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
        list_del(&timer->list);
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
 * wake_up_timer:
 *
 * Wake up time that need it.
 * This function should be called every second to work properly.
 */
void wake_up_timer(void) 
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










