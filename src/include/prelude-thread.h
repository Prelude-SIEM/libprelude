/*****
*
* Copyright (C) 2005 PreludeIDS Technologies. All Rights Reserved.
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

#ifndef _LIBPRELUDE_THREAD_H
#define _LIBPRELUDE_THREAD_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <pthread.h>
#include "prelude-inttypes.h"

#ifdef __cplusplus
 extern "C" {
#endif


int prelude_thread_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void));


/*
 * mutex
 */
int prelude_thread_mutex_lock(pthread_mutex_t *mutex);

int prelude_thread_mutex_unlock(pthread_mutex_t *mutex);

int prelude_thread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);

int prelude_thread_mutex_destroy(pthread_mutex_t *mutex);


/*
 * condition.
 */
int prelude_thread_cond_init(pthread_cond_t *cond, pthread_condattr_t *attr);

int prelude_thread_cond_signal(pthread_cond_t *cond);

int prelude_thread_cond_broadcast(pthread_cond_t *cond);

int prelude_thread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);

int prelude_thread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime);

int prelude_thread_cond_destroy(pthread_cond_t *cond);

/*
 * condition attribute
 */
int prelude_thread_condattr_init(pthread_condattr_t *attr);


/*
 *
 */
int prelude_thread_sigmask(int how, const sigset_t *newmask, sigset_t *oldmask);

int prelude_thread_create(pthread_t *thread, pthread_attr_t *attr, void *(*start_routine)(void *), void *arg);

void prelude_thread_exit(void *retval);

int prelude_thread_join(pthread_t th, void **thread_return);


/*
 *
 */
prelude_bool_t _prelude_thread_in_use(void);


#ifdef __cplusplus
 }
#endif

#endif
