#ifndef _LIBPRELUDE_THREADS_H
#define _LIBPRELUDE_THREADS_H

#include "config.h"

#ifdef ENABLE_PROFILING

/*
 * The following code was took and modified from
 * http://sam.zoy.org/doc/programming/gprof.html
 *
 * The authors is presumably Samuel Hocevar <sam@zoy.org>
 */


/* 
 * pthread_create wrapper for gprof compatibility
 *
 * needed headers: <pthread.h> <sys/time.h>
 */

typedef struct wrapper_s {
        void *(*start_routine)(void *);
        void *arg;

        pthread_mutex_t lock;
        pthread_cond_t  wait;

        struct itimerval itimer;
} thread_wrapper_t;




/*
 * The wrapper function in charge for setting the itimer value
 */
static void *wrapper_routine(void *data)
{
        void *arg;
        thread_wrapper_t *ptr = data;
        void *(*start_routine)(void *);
        
        /*
         * Put user data in thread-local variables
         */
        arg = ptr->arg;
        start_routine  = ptr->start_routine;
        
        /*
         * Set the profile timer value
         */
        setitimer(ITIMER_PROF, &ptr->itimer, NULL);

        /*
         * Tell the calling thread that we don't need its data anymore
         */
        pthread_mutex_lock(&ptr->lock);
        pthread_cond_signal(&ptr->wait);
        pthread_mutex_unlock(&ptr->lock);

        /*
         * Call the real function
         */
        return start_routine(arg);
}



/*
 * Same prototype as pthread_create; use some #define magic to
 * transparently replace it in other files
 */
static int gprof_pthread_create(pthread_t *thread, pthread_attr_t *attr,
                                void *(*start_routine)(void *), void *arg)
{
        int ret;
        thread_wrapper_t wrapper_data;

        /*
         * Initialize the wrapper structure
         */
        wrapper_data.arg = arg;
        wrapper_data.start_routine = start_routine;
    
        getitimer(ITIMER_PROF, &wrapper_data.itimer);
        
        pthread_cond_init(&wrapper_data.wait, NULL);
        pthread_mutex_init(&wrapper_data.lock, NULL);
        pthread_mutex_lock(&wrapper_data.lock);

        /*
         * The real pthread_create call
         */
        ret = pthread_create(thread, attr, &wrapper_routine, &wrapper_data);

        /*
         * If the thread was successfully spawned, wait for the data
         * to be released
         */
        if ( ret == 0 )
                pthread_cond_wait(&wrapper_data.wait, &wrapper_data.lock);

        pthread_mutex_unlock(&wrapper_data.lock);
        pthread_mutex_destroy(&wrapper_data.lock);
        pthread_cond_destroy(&wrapper_data.wait);

        return ret;
}

#define pthread_create gprof_pthread_create

#endif

#endif /* _LIBPRELUDE_THREADS_H */
