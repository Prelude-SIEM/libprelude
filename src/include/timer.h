/*****
*
* Copyright (C) 1998 - 2000, 2002 Vandoorselaere Yoann
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

#ifndef _TIMER_H
#define _TIMER_H

#include <stdlib.h>
#include <sys/time.h>

#include "list.h"


typedef struct {
        struct list_head list;

        short int expire;
        time_t start_time;

        void *data;
        void (*function)(void *data);
} prelude_timer_t;

/*
 * Use theses macros for compatibility purpose
 * if the internal timer structure change.
 */
#define timer_expire(timer) (timer)->expire
#define timer_data(timer) (timer)->data
#define timer_func(timer) (timer)->function

#define timer_set_expire(timer, x) timer_expire((timer)) = (x)
#define timer_set_data(timer, x) timer_data((timer)) = (x)
#define timer_set_callback(timer, x) timer_func((timer)) = (x)


/*
 * Init a timer (add it to the timer list).
 */
void timer_init(prelude_timer_t *timer);


/*
 * Destroy current timer.
 */
void timer_destroy_current(void);


/*
 * Reset current timer.
 */
void timer_reset_current(void);


/*
 * Reset timer 'timer'.
 */
void timer_reset(prelude_timer_t *timer);


/*
 * Destroy timer 'timer'.
 */
void timer_destroy(prelude_timer_t *timer);


/*
 * Return the time elapsed by timer 'timer'
 * from the last time it was reset'd or started.
 *
 * Store the result in a timeval structure given as argument.
 */
void timer_elapsed(prelude_timer_t *timer, struct timeval *tv);


/*
 * Wake up time that need it.
 * This function should be called every second to work properly.
 */
void prelude_wake_up_timer(void);


#endif
