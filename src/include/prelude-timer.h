/*****
*
* Copyright (C) 1998 - 2000, 2002, 2003 Yoann Vandoorselaere <yoann@prelude-ids.org>
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

#ifndef _LIBPRELUDE_PRELUDE_TIMER_H
#define _LIBPRELUDE_PRELUDE_TIMER_H


#include "prelude-list.h"


typedef struct {
        prelude_list_t list;

        int expire;
        time_t start_time;

        void *data;
        void (*function)(void *data);
} prelude_timer_t;

/*
 * Use theses macros for compatibility purpose
 * if the internal timer structure change.
 */
#define prelude_timer_get_expire(timer) (timer)->expire
#define prelude_timer_get_data(timer) (timer)->data
#define prelude_timer_get_callback(timer) (timer)->function

#define prelude_timer_set_expire(timer, x) prelude_timer_get_expire((timer)) = (x)
#define prelude_timer_set_data(timer, x) prelude_timer_get_data((timer)) = (x)
#define prelude_timer_set_callback(timer, x) prelude_timer_get_callback((timer)) = (x)


/*
 * Init a timer (add it to the timer list).
 */
void prelude_timer_init(prelude_timer_t *timer);


/*
 * Reset timer 'timer'.
 */
void prelude_timer_reset(prelude_timer_t *timer);


/*
 * Destroy timer 'timer'.
 */
void prelude_timer_destroy(prelude_timer_t *timer);


/*
 * Return the time elapsed by timer 'timer'
 * from the last time it was reset'd or started.
 *
 * Store the result in a timeval structure given as argument.
 */
void prelude_timer_elapsed(prelude_timer_t *timer, struct timeval *tv);


/*
 * Wake up time that need it.
 * This function should be called every second to work properly.
 */
void prelude_timer_wake_up(void);

/*
 *
 */
void prelude_timer_flush(void);


/*
 *
 */
void prelude_timer_lock_critical_region(void);


/*
 *
 */
void prelude_timer_unlock_critical_region(void);

#endif /* _LIBPRELUDE_PRELUDE_TIMER_H */
