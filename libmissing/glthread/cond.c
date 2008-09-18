/* Condition variables for multithreading.
   Copyright (C) 2008 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.  */

/* Written by Yoann Vandoorselaere <yoann@prelude-ids.org>, 2008.  */

#include <config.h>
#include <sys/time.h>

#include "glthread/cond.h"

/* ========================================================================= */

#if USE_WIN32_THREADS

/*
 * WIN32 implementation based on http://www.cs.wustl.edu/~schmidt/win32-cv-1.html
 */

int glthread_cond_init_func(gl_cond_t *cv)
{
  cv->waiters_count = 0;
  cv->wait_generation_count = 0;
  cv->release_count = 0;
  
  InitializeCriticalSection(&cv->lock);

  /* Create a manual-reset event. */
  cv->event = CreateEvent (NULL, TRUE, FALSE, NULL);
  cv->guard.done = 1;

  return 0;
}


static void 
init_once(gl_cond_t *cv)
{
  if (!cv->guard.done)
    {
      if (InterlockedIncrement (&cv->guard.started) == 0)
        /* This thread is the first one to need this lock.  Initialize it.  */
        glthread_cond_init (cv);
      else
        /* Yield the CPU while waiting for another thread to finish
           initializing this lock.  */
        while (!cv->guard.done)
          Sleep (0);
    }
}


int glthread_cond_destroy_func(gl_cond_t *cv)
{
  CloseHandle(cv->event);
  return 0;
}


static int cond_wait(gl_cond_t *cv, gl_lock_t *external_mutex, DWORD msec)
{
  int ret, my_generation, wait_done; 

  init_once(cv);

  EnterCriticalSection(&cv->lock);

  /* Increment count of waiters, 
    and store current generation in our activation record. */
  cv->waiters_count++;
  my_generation = cv->wait_generation_count;

  LeaveCriticalSection(&cv->lock);
  glthread_lock_unlock(external_mutex);

  do 
   {
    /* Wait until the event is signaled. */
    ret = WaitForSingleObject (cv->event, msec);
    if ( ret != WAIT_OBJECT_0 )
      break;

    EnterCriticalSection (&cv->lock);
    /* Exit the loop when the event is signaled and there are still
       waiting threads from this <wait_generation> that haven't been released from this wait yet.
     */
    wait_done = cv->release_count > 0 && cv->wait_generation_count != my_generation;
    LeaveCriticalSection (&cv->lock);
  } while (!wait_done);

  glthread_lock_lock(external_mutex);
  EnterCriticalSection(&cv->lock);
  cv->waiters_count--;

  if ( ret == WAIT_OBJECT_0 ) 
    {
      if ( --cv->release_count == 0 )
        /* We're the last waiter to be notified, so reset the manual event. */
        ResetEvent (cv->event);
    }

  LeaveCriticalSection (&cv->lock);

  return (ret == WAIT_TIMEOUT) ? ETIMEDOUT : (ret == WAIT_OBJECT_0) ? 0 : -1;
}


int glthread_cond_wait_func(gl_cond_t *cv, gl_lock_t *external_mutex)
{
  return cond_wait(cv, external_mutex, INFINITE);
}


/* Subtract the `struct timeval' values X and Y,
        storing the result in RESULT.
        Return 1 if the difference is negative, otherwise 0.  */
     
static int 
timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y)
{
  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_usec < y->tv_usec) 
    {
      int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
      y->tv_usec -= 1000000 * nsec;
      y->tv_sec += nsec;
    }
  if (x->tv_usec - y->tv_usec > 1000000) 
    {
      int nsec = (x->tv_usec - y->tv_usec) / 1000000;
      y->tv_usec += 1000000 * nsec;
      y->tv_sec -= nsec;
    }
     
  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;
     
  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}


int glthread_cond_timedwait_func(gl_cond_t *cv, gl_lock_t *external_mutex, struct timespec *ts)
{
  struct timeval res, now, end;

  end.tv_sec = ts->tv_sec;
  end.tv_usec = ts->tv_nsec / 1000;
  gettimeofday(&now, NULL);

  timeval_subtract(&res, &end, &now);

  return cond_wait(cv, external_mutex, (res.tv_sec * 1000) + (res.tv_usec / 1000));
}


int glthread_cond_broadcast_func(gl_cond_t *cv)
{
  init_once(cv);

  EnterCriticalSection(&cv->lock);
  /* The pthread_cond_broadcast() and pthread_cond_signal() functions  shall
     have no effect if there are no threads currently blocked on cond.
   */
  if (cv->waiters_count > 0) 
    {  
      SetEvent(cv->event);
      /* Release all the threads in this generation. */
      cv->release_count = cv->waiters_count;
      cv->wait_generation_count++;
    }
  LeaveCriticalSection(&cv->lock);

  return 0;
}

	
int glthread_cond_signal_func(gl_cond_t *cv)
{
  init_once(cv);

  EnterCriticalSection(&cv->lock);
  /* The pthread_cond_broadcast() and pthread_cond_signal() functions  shall
     have no effect if there are no threads currently blocked on cond.
   */
  if (cv->waiters_count > cv->release_count)
    {
      SetEvent(cv->event); /* Signal the manual-reset event.*/
      cv->release_count++;
      cv->wait_generation_count++;
    }
  LeaveCriticalSection(&cv->lock);

  return 0;
}


#endif

#if USE_PTH_THREADS

/* -------------------------- gl_cond_t datatype -------------------------- */

int
glthread_cond_timedwait_multithreaded (gl_cond_t *cond,
				       gl_lock_t *lock,
				       struct timespec *abstime)
{
  int ret, status;
  pth_event_t ev;

  ev = pth_event (PTH_EVENT_TIME, pth_time (abstime->tv_sec, abstime->tv_nsec / 1000));
  ret = pth_cond_await (cond, lock, ev);

  status = pth_event_status (ev);
  pth_event_free (ev, PTH_FREE_THIS);

  if (status == PTH_STATUS_OCCURRED)
    return ETIMEDOUT;

  return ret;
}

#endif

/* ========================================================================= */

#if USE_SOLARIS_THREADS

/* -------------------------- gl_cond_t datatype -------------------------- */

int
glthread_cond_timedwait_multithreaded (gl_cond_t *cond,
				       gl_lock_t *lock,
				       struct timespec *abstime)
{
  int ret;

  ret = cond_timedwait (cond, lock, abstime);
  if (ret == ETIME)
    return ETIMEDOUT;
  return ret;
}

#endif

/* ========================================================================= */
