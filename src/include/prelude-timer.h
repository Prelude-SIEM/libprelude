/*****
*
* Copyright (C) 1998-2005 PreludeIDS Technologies. All Rights Reserved.
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

#ifndef _LIBPRELUDE_PRELUDE_TIMER_H
#define _LIBPRELUDE_PRELUDE_TIMER_H

#include "prelude-config.h"

#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include "prelude-list.h"

#ifdef __cplusplus
 extern "C" {
#endif


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

void prelude_timer_init_list(prelude_timer_t *timer);


/*
 * Reset timer 'timer'.
 */
void prelude_timer_reset(prelude_timer_t *timer);


/*
 * Destroy timer 'timer'.
 */
void prelude_timer_destroy(prelude_timer_t *timer);


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


/*
 *
 */
int _prelude_timer_init(void);


void _prelude_timer_fork_prepare(void);
void _prelude_timer_fork_parent(void);
void _prelude_timer_fork_child(void);

#ifdef __cplusplus
 }
#endif

#endif /* _LIBPRELUDE_PRELUDE_TIMER_H */
