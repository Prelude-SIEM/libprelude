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

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>

#include "prelude.h"
#include "prelude-inttypes.h"
#include "prelude-thread.h"
#include "prelude-log.h"


#if PTHREAD_IN_USE_DETECTION_HARD
# define __prelude_thread_in_use() _prelude_thread_hard_in_use()
#endif


#ifdef USE_POSIX_THREADS_WEAK

# if !PTHREAD_IN_USE_DETECTION_HARD
#  pragma weak pthread_cancel
#  define __prelude_thread_in_use() (pthread_cancel != NULL)
# endif

#else

#  if !PTHREAD_IN_USE_DETECTION_HARD
#   define __prelude_thread_in_use() 1
#  endif

# endif


#ifndef HAVE_PTHREAD_ATFORK
# define pthread_atfork(prepare, parent, child) (0)
#endif


#ifndef HAVE_PTHREAD_CONDATTR_SETCLOCK
# define pthread_condattr_setclock(attr, clock_id) (0)
#endif


#define THR_FUNC(func)                                  \
        if ( use_thread ) return func; else return 0


#define THR_FUNC_NORETURN(func)                         \
        if ( use_thread ) func



static prelude_bool_t use_thread = FALSE;



int prelude_thread_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void))
{
        THR_FUNC(pthread_atfork(prepare, parent, child));
}


/*
 * mutex
 */
int prelude_thread_mutex_lock(pthread_mutex_t *mutex)
{
        THR_FUNC(pthread_mutex_lock(mutex));
}


int prelude_thread_mutex_unlock(pthread_mutex_t *mutex)
{
        THR_FUNC(pthread_mutex_unlock(mutex));
}


int prelude_thread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
        THR_FUNC(pthread_mutex_init(mutex, attr));
}


int prelude_thread_mutex_destroy(pthread_mutex_t *mutex)
{
        THR_FUNC(pthread_mutex_destroy(mutex));
}


/*
 * condition.
 */
int prelude_thread_cond_init(pthread_cond_t *cond, pthread_condattr_t *attr)
{
        THR_FUNC(pthread_cond_init(cond, attr));
}



int prelude_thread_cond_signal(pthread_cond_t *cond)
{
        THR_FUNC(pthread_cond_signal(cond));
}



int prelude_thread_cond_broadcast(pthread_cond_t *cond)
{
        THR_FUNC(pthread_cond_broadcast(cond));
}


int prelude_thread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
        THR_FUNC(pthread_cond_wait(cond, mutex));
}


int prelude_thread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime)
{
        THR_FUNC(pthread_cond_timedwait(cond, mutex, abstime));
}


int prelude_thread_cond_destroy(pthread_cond_t *cond)
{
        THR_FUNC(pthread_cond_destroy(cond));
}


int prelude_thread_condattr_init(pthread_condattr_t *attr) 
{
        THR_FUNC(pthread_condattr_init(attr));
}



/*
 *
 */
int prelude_thread_create(pthread_t *thread, pthread_attr_t *attr, void *(*start_routine)(void *), void *arg)
{
        THR_FUNC(pthread_create(thread, attr, start_routine, arg));
}


void prelude_thread_exit(void *retval)
{
        THR_FUNC_NORETURN(pthread_exit(retval));
}


int prelude_thread_join(pthread_t th, void **thread_return)
{
        THR_FUNC(pthread_join(th, thread_return));
}


int prelude_thread_sigmask(int how, const sigset_t *newmask, sigset_t *oldmask)
{
        THR_FUNC(pthread_sigmask(how, newmask, oldmask));
}



/*
 *
 */
int prelude_thread_init(void *nil)
{
        use_thread = TRUE;
        return 0;
}



/*
 * Pthread stubs detection code for Solaris/Hp from GnuLib.
 */
#ifdef PTHREAD_IN_USE_DETECTION_HARD

static void *dummy_thread_func(void *arg)
{
        return arg;
}



static prelude_bool_t _prelude_thread_hard_in_use(void)
{
        int ret;
        void *retval;
        pthread_t thread;
                
        ret = pthread_create(&thread, NULL, dummy_thread_func, NULL);
        if ( ret != 0 ) 
                /* we're using libc stubs */
                return FALSE;

        /*
         * Real pthread in use.
         */
        ret = pthread_join(thread, &retval);
        if ( ret != 0 )
                abort();
        
        return TRUE;
}

#endif



prelude_bool_t _prelude_thread_in_use(void)
{
        static prelude_bool_t tested = FALSE;
        
        if ( tested )
                return use_thread;
        
        use_thread = __prelude_thread_in_use();
        tested = TRUE;
        
        prelude_log(PRELUDE_LOG_DEBUG, "[init] thread used=%d\n", use_thread);
        
        return use_thread;
}

