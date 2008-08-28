/*****
*
* Copyright (C) 1999-2005 PreludeIDS Technologies. All Rights Reserved.
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
#include <time.h>
#include <assert.h>

#include "glthread/thread.h"
#include "glthread/lock.h"
#include "glthread/cond.h"

#include "prelude-log.h"
#include "prelude-list.h"
#include "prelude-linked-object.h"
#include "prelude-async.h"
#include "prelude-error.h"

#include "prelude-timer.h"


static PRELUDE_LIST(timer_list);
static gl_lock_t mutex = gl_lock_initializer;



inline static void timer_lock_list(void)
{
        gl_lock_lock(mutex);
}



inline static void timer_unlock_list(void)
{
        gl_lock_unlock(mutex);
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

        if ( now == -1 || time_elapsed(timer, now) >= prelude_timer_get_expire(timer) ) {
                timer->start_time = -1;

                prelude_timer_get_callback(timer)(prelude_timer_get_data(timer));

                return 0;
        }

        return -1;
}




static prelude_timer_t *get_next_timer(void)
{
        prelude_list_t *tmp;
        prelude_timer_t *timer = NULL;

        gl_lock_lock(mutex);

        prelude_list_for_each(&timer_list, tmp) {
                timer = prelude_list_entry(tmp, prelude_timer_t, list);
                break;
        }

        gl_lock_unlock(mutex);

        return timer;
}



/*
 * Walk the list of timer,
 * call the wake_up_if_need_function on each timer.
 */
static void walk_and_wake_up_timer(time_t now)
{
        int ret, woke = 0;
        prelude_timer_t *timer;

        while ( (timer = get_next_timer()) ) {

                ret = wake_up_if_needed(timer, now);
                if ( ret < 0 )
                        break;

                woke++;
        }

        prelude_log_debug(5, "woke up %d timer\n", woke);
}



/*
 * search the timer list forward for the timer entry
 * that should be before our inserted timer.
 */
static prelude_list_t *search_previous_forward(prelude_timer_t *timer, time_t expire)
{
        int hop = 0;
        prelude_timer_t *cur;
        prelude_list_t *tmp, *prev = NULL;

        prelude_list_for_each(&timer_list, tmp) {
                cur = prelude_list_entry(tmp, prelude_timer_t, list);

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
                        prelude_log_debug(5, "[expire=%d] found forward in %d hop at %p\n", timer->expire, hop, cur);
                        return tmp;
                }

                else if ( (cur->start_time + cur->expire) > expire ) {
                        /*
                         * we found a timer expiring after us. We can return
                         * the previously saved entry.
                         */
                        prelude_log_debug(5, "[expire=%d] found forward in %d hop at %p\n", timer->expire, hop, cur);
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
static prelude_list_t *search_previous_backward(prelude_timer_t *timer, time_t expire)
{
        int hop = 0;
        prelude_timer_t *cur;
        prelude_list_t *tmp;

        for ( tmp = timer_list.prev; tmp != &timer_list; tmp = tmp->prev ) {

                cur = prelude_list_entry(tmp, prelude_timer_t, list);

                if ( (cur->start_time + cur->expire) <= expire ) {
                        prelude_log_debug(5, "[expire=%d] found backward in %d hop at %p\n", timer->expire, hop + 1, cur);
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
        return prelude_list_entry(timer_list.next, prelude_timer_t, list);
}




inline static prelude_timer_t *get_last_timer(void)
{
        return prelude_list_entry(timer_list.prev, prelude_timer_t, list);
}



/*
 * On entering in this function, we know that :
 * - expire is > than first_expire.
 * - expire is < than last_expire.
 */
static prelude_list_t *search_previous_timer(prelude_timer_t *timer)
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
                prelude_log_debug(5, "[expire=%d] found without search (insert last)\n", timer->expire);
                return timer_list.prev;
        }

        /*
         * timer we want to insert expire before (or at the same time), the known
         * to be expiring first timer. This mean we should insert the new timer at
         * the beginning of the list.
         */
        if ( timer->expire <= time_remaining(first, timer->start_time) ) {
                prelude_log_debug(5, "[expire=%d] found without search (insert first)\n", timer->expire);
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



static void timer_destroy_unlocked(prelude_timer_t *timer)
{
        if ( ! prelude_list_is_empty(&timer->list) )
                prelude_list_del_init(&timer->list);
}




static void timer_init_unlocked(prelude_timer_t *timer)
{
        prelude_list_t *prev;

        timer->start_time = time(NULL);

        if ( ! prelude_list_is_empty(&timer_list) )
                prev = search_previous_timer(timer);
        else
                prev = &timer_list;

        prelude_list_add(prev, &timer->list);
}




/**
 * prelude_timer_init:
 * @timer: timer to initialize.
 *
 * Initialize a timer (add it to the timer list).
 */
void prelude_timer_init(prelude_timer_t *timer)
{
        timer_lock_list();
        timer_init_unlocked(timer);
        timer_unlock_list();
}



/**
 * prelude_timer_init_list:
 * @timer: Pointer to a #prelude_timer_t object.
 *
 * Initialize @timer list member. This is useful if
 * you're going to call prelude_timer_destroy() on timer
 * for which prelude_timer_init() was never called.
 *
 */
void prelude_timer_init_list(prelude_timer_t *timer)
{
        prelude_list_init(&timer->list);
}



/**
 * prelude_timer_reset:
 * @timer: the timer to reset.
 *
 * Reset timer 'timer', as if it was just started.
 */
void prelude_timer_reset(prelude_timer_t *timer)
{
        timer_lock_list();

        timer_destroy_unlocked(timer);
        timer_init_unlocked(timer);

        timer_unlock_list();
}




/**
 * prelude_timer_destroy:
 * @timer: the timer to destroy.
 *
 * Destroy the timer 'timer',
 * this remove it from the active timer list.
 */
void prelude_timer_destroy(prelude_timer_t *timer)
{
        timer_lock_list();
        timer_destroy_unlocked(timer);
        timer_unlock_list();
}




/**
 * prelude_timer_wake_up:
 *
 * Wake up timer that need it.
 * This function should be called every second to work properly.
 */
void prelude_timer_wake_up(void)
{
        time_t now = time(NULL);

        walk_and_wake_up_timer(now);
}




/**
 * prelude_timer_flush:
 *
 * Expire every timer.
 */
void prelude_timer_flush(void)
{
        walk_and_wake_up_timer(-1);
}




/**
 * prelude_timer_lock_critical_region:
 *
 * Deactivate timer wake-up until timer_unlock_critical_region() is called.
 */
void prelude_timer_lock_critical_region(void)
{
        timer_lock_list();
}



/**
 * prelude_timer_unlock_critical_region:
 *
 * Reactivate timer wake-up after timer_lock_critical_regions() has been called.
 */
void prelude_timer_unlock_critical_region(void)
{
        timer_unlock_list();
}



int _prelude_timer_init(void)
{
        return 0;
}



void _prelude_timer_fork_prepare(void)
{
        prelude_timer_lock_critical_region();
}


void _prelude_timer_fork_parent(void)
{
        prelude_timer_unlock_critical_region();
}


void _prelude_timer_fork_child(void)
{
        prelude_list_init(&timer_list);
        gl_lock_init(mutex);
}
