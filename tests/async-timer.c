#include "config.h"

#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include "prelude.h"

#include "glthread/lock.h"


struct asyncobj {
        PRELUDE_ASYNC_OBJECT;
        int myval;
};


static int async_done = 0;
static int timer_count = 0;
static gl_lock_t lock = gl_lock_initializer;


static void timer_cb(void *data)
{
        gl_lock_lock(lock);
        timer_count++;
        prelude_timer_reset(data);
        gl_lock_unlock(lock);
}


static void async_func(void *obj, void *data)
{
        struct asyncobj *ptr = obj;

        gl_lock_lock(lock);
        async_done = 1;
        assert(ptr->myval == 10);
        gl_lock_unlock(lock);
}


int main(void)
{
        prelude_timer_t timer;
        struct asyncobj myobj;

        assert(prelude_init(NULL, NULL) == 0);
        assert(prelude_async_init() == 0);
        prelude_async_set_flags(PRELUDE_ASYNC_FLAGS_TIMER);

        prelude_timer_set_expire(&timer, 1);
        prelude_timer_set_data(&timer, &timer);
        prelude_timer_set_callback(&timer, timer_cb);
        prelude_timer_init(&timer);

        sleep(3);

        gl_lock_lock(lock);
        assert(timer_count >= 2);
        gl_lock_unlock(lock);

        myobj.myval = 10;
        prelude_async_set_callback((prelude_async_object_t *) &myobj, async_func);
        prelude_async_add((prelude_async_object_t *) &myobj);

        prelude_async_exit();
        assert(async_done);

        exit(0);
}
