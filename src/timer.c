/*****
*
* Copyright (C) 1999 - 2003 Yoann Vandoorselaere <yoann@prelude-ids.org>
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
#include "prelude-log.h"


#ifdef DEBUG
 #define dprint(args...) fprintf(stderr, args)
#else
 #define dprint(args...)
#endif


static int count = 0;
static int async_timer = 0;
static LIST_HEAD(timer_list);
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;



inline static void timer_lock(void) 
{
        if ( async_timer )
                pthread_mutex_lock(&mutex);
}



inline static void timer_unlock(void) 
{
        if ( async_timer )
                pthread_mutex_unlock(&mutex);
}




static void set_expiration_time(prelude_timer_t *timer) 
{
        timer->start_time = time(NULL);
}




/*
 * Return the time elapsed by a timer 'timer' from now,
 * to the time it was created / reset.
 */
static time_t time_elapsed(prelude_timer_t *timer, time_t now) 
{
        return now - timer->start_time;
}




static time_t time_remaining(prelude_timer_t *timer, time_t now)
{
        return timer->expire - time_elapsed(timer, now);
}




/*
 * If timer 'timer' need to be waked up (it elapsed >= time
 * for it to expire), call it's callback function, with it's
 * registered argument.
 *
 * All expired timer should be destroyed.
 */
static int wake_up_if_needed(prelude_timer_t *timer, time_t now) 
{
        assert(timer->start_time != -1);

        if ( now == -1 || time_elapsed(timer, now) >= timer_expire(timer) ) {
                timer->start_time = -1;
                timer_func(timer)(timer_data(timer));
                return 0;
        }
        
        return -1;
}




/*
 * Walk the list of timer,
 * call the wake_up_if_need_function on each timer.
 */
static void walk_and_wake_up_timer(time_t now) 
{
        int ret, woke = 0;
        prelude_timer_t *timer;
        struct list_head *tmp, *next;

        timer_lock();
        next = ( list_empty(&timer_list) ) ? NULL : timer_list.next;
        timer_unlock();
                
        while ( next ) {
                tmp = next;

                timer_lock();
                next = ( tmp->next != &timer_list ) ? tmp->next : NULL;
                timer_unlock();
                
                timer = list_entry(tmp, prelude_timer_t, list);

                ret = wake_up_if_needed(timer, now);
                if ( ret < 0 )
                        /*
                         * no timer remaining in the list will be expired.
                         */
                        break;

                woke++;
        }

        dprint("woke up %d/%d timer\n", woke, count);
}



/*
 * search the timer list forward for the timer entry
 * that should be before our inserted timer.
 */
static struct list_head *search_previous_forward(prelude_timer_t *timer, time_t expire) 
{
        int hop = 0;
        prelude_timer_t *cur;
        struct list_head *tmp, *prev = NULL;
        
        list_for_each(tmp, &timer_list) {
                cur = list_entry(tmp, prelude_timer_t, list);

                hop++;

                if ( (cur->start_time + cur->expire) < expire ) {
                        /*
                         * we found a previous timer (expiring before us),
                         * but we're walking the list forward, and there could be more...
                         * save and continue.
                         */
                        prev = tmp;
                        continue; 
                }

                else if ( (cur->start_time + cur->expire) == expire ) {
                        /*
                         * we found a timer that's expiring at the same time
                         * as us. Return it as the previous insertion point.
                         */
                        dprint("[expire=%d] found forward in %d hop at %p\n", timer->expire, hop, cur);
                        return tmp;
                }

                else if ( (cur->start_time + cur->expire) > expire ) {
                        /*
                         * we found a timer expiring after us. We can return 
                         * the previously saved entry.
                         */
                        dprint("[expire=%d] found forward in %d hop at %p\n", timer->expire, hop, cur);
                        assert(prev);
                        return prev;
                }
        }

        /*
         * this should never happen, as search_previous_timer verify
         * if timer should be inserted last.
         */
        abort();
}




/*
 * search the timer list backward for the timer entry
 * that should be before our inserted timer.
 */
static struct list_head *search_previous_backward(prelude_timer_t *timer, time_t expire) 
{
        int hop = 0;
        prelude_timer_t *cur;
        struct list_head *tmp;
        
        for ( tmp = timer_list.prev; tmp != &timer_list; tmp = tmp->prev ) {
                
                cur = list_entry(tmp, prelude_timer_t, list);
                
                if ( (cur->start_time + cur->expire) <= expire ) {
                        dprint("[expire=%d] found backward in %d hop at %p\n", timer->expire, hop + 1, cur);
                        assert(tmp);
                        return tmp;
                }

                hop++;
        }

        /*
         * this should never happen, as search_previous_timer verify
         * if timer should be inserted first.
         */
        abort();
}




inline static prelude_timer_t *get_first_timer(void) 
{
        return list_entry(timer_list.next, prelude_timer_t, list);
}




inline static prelude_timer_t *get_last_timer(void) 
{
        return list_entry(timer_list.prev, prelude_timer_t, list);
}



/*
 * On entering in this function, we know that :
 * - expire is > than first_expire.
 * - expire is < than last_expire.
 */
static struct list_head *search_previous_timer(prelude_timer_t *timer) 
{
        time_t expire;
        prelude_timer_t *last, *first;
        time_t last_remaining, first_remaining;

        last = get_last_timer();
        first = get_first_timer();

        /*
         * timer we want to insert expire after (or at the same time) the known
         * to be expiring last timer. This mean we should insert the new timer
         * at the end of the list.
         */
        if ( timer->expire >= time_remaining(last, timer->start_time) ) {
                assert(timer_list.prev);
                dprint("[expire=%d] found without search (insert last)\n", timer->expire);
                return timer_list.prev;
        }
        
        /*
         * timer we want to insert expire before (or at the same time), the known
         * to be expiring first timer. This mean we should insert the new timer at
         * the beginning of the list.
         */
        if ( timer->expire <= time_remaining(first, timer->start_time) ) {
                assert(&timer_list);
                dprint("[expire=%d] found without search (insert first)\n", timer->expire);
                return &timer_list;
        }

        /*
         * we now know we expire after the first expiring timer,
         * but before the last expiring one. 
         *
         * compute expiration time for current, last, and first timer.
         */
        expire = timer->expire + timer->start_time;        
        last_remaining = time_remaining(last, timer->start_time);
        first_remaining = time_remaining(first, timer->start_time);

        /*
         * use the better list iterating function to find the previous timer.
         */
        if ( (last_remaining - timer->expire) > (timer->expire - first_remaining) )
                /*
                 * previous is probably near the beginning of the list.
                 */
                return search_previous_forward(timer, timer->expire + timer->start_time);
        else
                /*
                 * previous is probably near the end of the list.
                 */
                return search_previous_backward(timer, timer->expire + timer->start_time);
}





/**
 * timer_init:
 * @timer: timer to initialize.
 *
 * Initialize a timer (add it to the timer list).
 */
void timer_init(prelude_timer_t *timer)
{
        struct list_head *prev;
        
        set_expiration_time(timer);

        timer_lock();
        count++;
        
        if ( ! list_empty(&timer_list) ) {
                prev = search_previous_timer(timer);
        } else
                prev = &timer_list;

        list_add(&timer->list, prev);

        timer_unlock();
}



/**
 * timer_reset:
 * @timer: the timer to reset.
 *
 * Reset timer 'timer', as if it was just started.
 */
void timer_reset(prelude_timer_t *timer) 
{
        timer_destroy(timer);
        timer_init(timer);
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
        timer_lock();
        
        count--;
        list_del(&timer->list);

        timer_unlock();
}




/**
 * prelude_wake_up_timer:
 *
 * Wake up timer that need it.
 * This function should be called every second to work properly.
 */
void prelude_wake_up_timer(void) 
{
        time_t now = time(NULL);
        
        walk_and_wake_up_timer(now);
}





/**
 * timer_flush:
 *
 * Expire every timer.
 */
void timer_flush(void) 
{
        walk_and_wake_up_timer(-1);
}




