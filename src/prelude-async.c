#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <inttypes.h>
#include <pthread.h>


#include "prelude-list.h"
#include "timer.h"
#include "common.h"
#include "prelude-io.h"
#include "prelude-message.h"
#include "prelude-async.h"


static LIST_HEAD(joblist);

static pthread_t thread;
static pthread_cond_t cond;
static pthread_mutex_t mutex;



static void wait_timer_and_data(void) 
{
        int ret = 0;
        struct timespec ts;
        struct timeval now;
        
        /*
         * Setup the condition timer.
         */ 
        gettimeofday(&now, NULL);
        ts.tv_sec = now.tv_sec + 1;
        ts.tv_nsec = now.tv_usec * 1000;
        
        pthread_mutex_lock(&mutex);
        while ( list_empty(&joblist) && ret != ETIMEDOUT ) {
                ret = pthread_cond_timedwait(&cond, &mutex, &ts);
        }
        pthread_mutex_unlock(&mutex);
        
        if ( ret == ETIMEDOUT ) {
                prelude_wake_up_timer();
                wait_timer_and_data(); /* tail recursion */
        }
}




static void async_thread(void) 
{
        prelude_async_object_t *obj;
        struct list_head *tmp, *bkp;
        
        
        while ( 1 ) {
                
                wait_timer_and_data();
                
                for ( tmp = joblist.next; tmp != &joblist; tmp = bkp ) {
                    
                        obj = prelude_list_get_object(tmp, prelude_async_object_t);
                        bkp = tmp->next;
                        prelude_async_del(obj);
                        obj->func(obj, obj->data);
                }
        }
}




static void prelude_async_exit(void)  
{
        log(LOG_INFO, "Flushing remaining messages.\n");
        
        pthread_cancel(thread);
        pthread_cond_destroy(&cond);
        pthread_mutex_destroy(&mutex);
}




/**
 * prelude_async_init:
 */
int prelude_async_init(void) 
{
        int ret;
        
        pthread_mutex_init(&mutex, NULL);
        pthread_cond_init(&cond, NULL);
        
        ret = pthread_create(&thread, NULL, async_thread, NULL);
        if ( ret < 0 ) {
                pthread_cond_destroy(&cond);
                pthread_mutex_destroy(&mutex);
        }

        return atexit(prelude_async_exit);
}





/**
 * prelude_async_queue:
 */
void prelude_async_add(prelude_async_object_t *obj) 
{
        pthread_mutex_lock(&mutex);
        prelude_list_add_tail((prelude_linked_object_t *)obj, &joblist);
        pthread_mutex_unlock(&mutex);
}



void prelude_async_del(prelude_async_object_t *obj) 
{
        pthread_mutex_lock(&mutex);
        prelude_list_del((prelude_linked_object_t *)obj);
        pthread_mutex_unlock(&mutex);
}
