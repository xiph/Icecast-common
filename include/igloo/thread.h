/* thread.h
 * - Thread Abstraction Function Headers
 *
 * Copyright (C) 2014       Michael Smith <msmith@icecast.org>,
 *                          Brendan Cully <brendan@xiph.org>,
 *                          Karl Heyes <karl@xiph.org>,
 *                          Jack Moffitt <jack@icecast.org>,
 * Copyright (C) 2012-2018  Philipp "ph3-der-loewe" Schafft <lion@lion.leolix.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 */

#ifndef _LIBIGLOO__THREAD_H_
#define _LIBIGLOO__THREAD_H_

#include "config.h"

#include <pthread.h>

/* renamed from thread_t due to conflict on OS X */

typedef struct {
    /* the local id for the thread, and it's name */
    long thread_id;
    char *name;

    /* the time the thread was created */
    time_t create_time;
    
    /* the file and line which created this thread */
    char *file;
    int line;

    /* is the thread running detached? */
    int detached;

    /* the system specific thread */
    pthread_t sys_thread;
} igloo_thread_type;

typedef struct {
#ifdef DEBUG_MUTEXES
    /* the local id and name of the mutex */
    long mutex_id;
    char *name;

    /* the thread which is currently locking this mutex */
    long thread_id;

    /* the file and line where the mutex was locked */
    char *file;
    int line;    

#endif

    /* the system specific mutex */
    pthread_mutex_t sys_mutex;
} igloo_mutex_t;

typedef struct {
#ifdef THREAD_DEBUG
    long cond_id;
    char *name;
#endif

    pthread_mutex_t cond_mutex;
    pthread_cond_t sys_cond;
} igloo_cond_t;

typedef struct {
#ifdef THREAD_DEBUG
    long rwlock_id;
    char *name;

    /* information on which thread and where in the code
    ** this rwlock was write locked
    */
    long thread_id;
    char *file;
    int line;
#endif

    pthread_rwlock_t sys_rwlock;
} igloo_rwlock_t;

#ifdef IGLOO_CTC_HAVE_PTHREAD_SPIN_LOCK
typedef struct
{
    pthread_spinlock_t lock;
} igloo_spin_t;

void igloo_thread_spin_create (igloo_spin_t *spin);
void igloo_thread_spin_destroy (igloo_spin_t *spin);
void igloo_thread_spin_lock (igloo_spin_t *spin);
void igloo_thread_spin_unlock (igloo_spin_t *spin);
#else
typedef igloo_mutex_t igloo_spin_t;
#define igloo_thread_spin_create(x)  thread_mutex_create(x)
#define igloo_thread_spin_destroy(x)   igloo_thread_mutex_destroy(x)
#define igloo_thread_spin_lock(x)      thread_mutex_lock(x)
#define igloo_thread_spin_unlock(x)    thread_mutex_unlock(x)
#endif

#define igloo_thread_create(n,x,y,z) igloo_thread_create_c(n,x,y,z,__LINE__,__FILE__)
#define igloo_thread_mutex_create(x) igloo_thread_mutex_create_c(x,__LINE__,__FILE__)
#define igloo_thread_mutex_lock(x) igloo_thread_mutex_lock_c(x,__LINE__,__FILE__)
#define igloo_thread_mutex_unlock(x) igloo_thread_mutex_unlock_c(x,__LINE__,__FILE__)
#define igloo_thread_cond_create(x) igloo_thread_cond_create_c(x,__LINE__,__FILE__)
#define igloo_thread_cond_signal(x) igloo_thread_cond_signal_c(x,__LINE__,__FILE__)
#define igloo_thread_cond_broadcast(x) igloo_thread_cond_broadcast_c(x,__LINE__,__FILE__)
#define igloo_thread_cond_wait(x) igloo_thread_cond_wait_c(x,__LINE__,__FILE__)
#define igloo_thread_cond_timedwait(x,t) igloo_thread_cond_wait_c(x,t,__LINE__,__FILE__)
#define igloo_thread_rwlock_create(x) igloo_thread_rwlock_create_c(x,__LINE__,__FILE__)
#define igloo_thread_rwlock_rlock(x) igloo_thread_rwlock_rlock_c(x,__LINE__,__FILE__)
#define igloo_thread_rwlock_wlock(x) igloo_thread_rwlock_wlock_c(x,__LINE__,__FILE__)
#define igloo_thread_rwlock_unlock(x) igloo_thread_rwlock_unlock_c(x,__LINE__,__FILE__)
#define igloo_thread_exit(x) igloo_thread_exit_c(x,__LINE__,__FILE__)

#define igloo_MUTEX_STATE_NOTLOCKED -1
#define igloo_MUTEX_STATE_NEVERLOCKED -2
#define igloo_MUTEX_STATE_UNINIT -3
#define igloo_THREAD_DETACHED 1
#define igloo_THREAD_ATTACHED 0

/* init/shutdown of the library */
void igloo_thread_initialize(void);
void igloo_thread_initialize_with_log_id(int log_id);
void igloo_thread_shutdown(void);

/* creation, destruction, locking, unlocking, signalling and waiting */
igloo_thread_type *igloo_thread_create_c(char *name, void *(*start_routine)(void *), 
        void *arg, int detached, int line, char *file);
void igloo_thread_mutex_create_c(igloo_mutex_t *mutex, int line, char *file);
void igloo_thread_mutex_lock_c(igloo_mutex_t *mutex, int line, char *file);
void igloo_thread_mutex_unlock_c(igloo_mutex_t *mutex, int line, char *file);
void igloo_thread_mutex_destroy(igloo_mutex_t *mutex);
void igloo_thread_cond_create_c(igloo_cond_t *cond, int line, char *file);
void igloo_thread_cond_signal_c(igloo_cond_t *cond, int line, char *file);
void igloo_thread_cond_broadcast_c(igloo_cond_t *cond, int line, char *file);
void igloo_thread_cond_wait_c(igloo_cond_t *cond, int line, char *file);
void igloo_thread_cond_timedwait_c(igloo_cond_t *cond, int millis, int line, char *file);
void igloo_thread_cond_destroy(igloo_cond_t *cond);
void igloo_thread_rwlock_create_c(igloo_rwlock_t *rwlock, int line, char *file);
void igloo_thread_rwlock_rlock_c(igloo_rwlock_t *rwlock, int line, char *file);
void igloo_thread_rwlock_wlock_c(igloo_rwlock_t *rwlock, int line, char *file);
void igloo_thread_rwlock_unlock_c(igloo_rwlock_t *rwlock, int line, char *file);
void igloo_thread_rwlock_destroy(igloo_rwlock_t *rwlock);
void igloo_thread_exit_c(long val, int line, char *file);

/* sleeping */
void igloo_thread_sleep(unsigned long len);

/* for using library functions which aren't threadsafe */
void igloo_thread_library_lock(void);
void igloo_thread_library_unlock(void);
#define IGLOO_PROTECT_CODE(code) { igloo_thread_library_lock(); code; igloo_thread_library_unlock(); }

/* thread information functions */
igloo_thread_type *igloo_thread_self(void);

/* renames current thread */
void igloo_thread_rename(const char *name);

/* waits until thread_exit is called for another thread */
void igloo_thread_join(igloo_thread_type *thread);

#endif  /* __THREAD_H__ */
