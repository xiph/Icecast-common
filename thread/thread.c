/* threads.c: Thread Abstraction Functions
 *
 * Copyright (C) 2014       Michael Smith <msmith@icecast.org>,
 *                          Brendan Cully <brendan@xiph.org>,
 *                          Karl Heyes <karl@xiph.org>,
 *                          Jack Moffitt <jack@icecast.org>,
 *                          Ed "oddsock" Zaleski <oddsock@xiph.org>,
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

#ifdef HAVE_CONFIG_H
 #include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>

#include <pthread.h>

#ifndef _WIN32
#include <unistd.h>
#include <sys/time.h>
#else
#include <windows.h>
#include <winbase.h>
#endif

#include <signal.h>

#include <igloo/thread.h>
#include <igloo/avl.h>
#ifdef THREAD_DEBUG
#include <igloo/log.h>
#endif

#ifdef _WIN32
#define __FUNCTION__ __FILE__
#endif

#ifdef THREAD_DEBUG
#define CATMODULE "thread"
#define LOG_ERROR(y) igloo_log_write(_logid, 1, CATMODULE "/", __FUNCTION__, y)
#define LOG_ERROR3(y, z1, z2, z3) igloo_log_write(_logid, 1, CATMODULE "/", __FUNCTION__, y, z1, z2, z3)
#define LOG_ERROR7(y, z1, z2, z3, z4, z5, z6, z7) igloo_log_write(_logid, 1, CATMODULE "/", __FUNCTION__, y, z1, z2, z3, z4, z5, z6, z7)

#define LOG_WARN(y) igloo_log_write(_logid, 2, CATMODULE "/", __FUNCTION__, y)
#define LOG_WARN3(y, z1, z2, z3) igloo_log_write(_logid, 2, CATMODULE "/", __FUNCTION__, y, z1, z2, z3)
#define LOG_WARN5(y, z1, z2, z3, z4, z5) igloo_log_write(_logid, 2, CATMODULE "/", __FUNCTION__, y, z1, z2, z3, z4, z5)
#define LOG_WARN7(y, z1, z2, z3, z4, z5, z6, z7) igloo_log_write(_logid, 2, CATMODULE "/", __FUNCTION__, y, z1, z2, z3, z4, z5, z6, z7)

#define LOG_INFO(y) igloo_log_write(_logid, 3, CATMODULE "/", __FUNCTION__, y)
#define LOG_INFO4(y, z1, z2, z3, z4) igloo_log_write(_logid, 3, CATMODULE "/", __FUNCTION__, y, z1, z2, z3, z4)
#define LOG_INFO5(y, z1, z2, z3, z4, z5) igloo_log_write(_logid, 3, CATMODULE "/", __FUNCTION__, y, z1, z2, z3, z4, z5)

#define LOG_DEBUG(y) igloo_log_write(_logid, 4, CATMODULE "/", __FUNCTION__, y)
#define LOG_DEBUG2(y, z1, z2) igloo_log_write(_logid, 4, CATMODULE "/", __FUNCTION__, y, z1, z2)
#define LOG_DEBUG5(y, z1, z2, z3, z4, z5) igloo_log_write(_logid, 4, CATMODULE "/", __FUNCTION__, y, z1, z2, z3, z4, z5)
#endif

/* thread starting structure */
typedef struct thread_start_tag {
    /* the real start routine and arg */
    void *(*start_routine)(void *);
    void *arg;

    /* the other stuff we need to make sure this thread is inserted into
    ** the thread tree
    */
    igloo_thread_type *thread;
    pthread_t sys_thread;
} thread_start_t;

static long igloo__next_thread_id = 0;
static int igloo__initialized = 0;
static igloo_avl_tree *igloo__threadtree = NULL;

#ifdef DEBUG_MUTEXES
static igloo_mutex_t igloo__threadtree_mutex = { -1, NULL, igloo_MUTEX_STATE_UNINIT, NULL, -1,
    PTHREAD_MUTEX_INITIALIZER};
#else
static igloo_mutex_t igloo__threadtree_mutex = { PTHREAD_MUTEX_INITIALIZER };
#endif



#ifdef DEBUG_MUTEXES
static int _logid = -1;
static long _next_mutex_id = 0;

static igloo_avl_tree *_mutextree = NULL;
static igloo_mutex_t _mutextree_mutex = { -1, NULL, igloo_MUTEX_STATE_UNINIT, NULL, -1,
    PTHREAD_MUTEX_INITIALIZER};
#endif

#ifdef DEBUG_MUTEXES
static igloo_mutex_t igloo__library_mutex = { -1, NULL, igloo_MUTEX_STATE_UNINIT, NULL, -1,
    PTHREAD_MUTEX_INITIALIZER};
#else
static igloo_mutex_t igloo__library_mutex = { PTHREAD_MUTEX_INITIALIZER };
#endif

/* INTERNAL FUNCTIONS */

/* avl tree functions */
#ifdef DEBUG_MUTEXES
static int _compare_mutexes(void *compare_arg, void *a, void *b);
static int _free_mutex(void *key);
#endif

static int igloo__compare_threads(void *compare_arg, void *a, void *b);
static int igloo__free_thread(void *key);

/* mutex fuctions */
static void _mutex_create(igloo_mutex_t *mutex);
static void _mutex_lock(igloo_mutex_t *mutex);
static void _mutex_unlock(igloo_mutex_t *mutex);

/* misc thread stuff */
static void *igloo__start_routine(void *arg);
static void _catch_signals(void);
static void _block_signals(void);

/* LIBRARY INITIALIZATION */

void igloo_thread_initialize(void)
{
    igloo_thread_type *thread;

    /* set up logging */

#ifdef THREAD_DEBUG
    igloo_log_initialize();
    _logid = igloo_log_open("thread.log");
    igloo_log_set_level(_logid, THREAD_DEBUG);
#endif

#ifdef DEBUG_MUTEXES
    /* create all the internal mutexes, and initialize the mutex tree */

    _mutextree = igloo_avl_tree_new(_compare_mutexes, NULL);

    /* we have to create this one by hand, because there's no
    ** mutextree_mutex to lock yet!
    */
    _mutex_create(&_mutextree_mutex);

    _mutextree_mutex.mutex_id = _next_mutex_id++;
    igloo_avl_insert(_mutextree, (void *)&_mutextree_mutex);
#endif

    igloo_thread_mutex_create(&igloo__threadtree_mutex);
    igloo_thread_mutex_create(&igloo__library_mutex);

    /* initialize the thread tree and insert the main thread */

    igloo__threadtree = igloo_avl_tree_new(igloo__compare_threads, NULL);

    thread = (igloo_thread_type *)malloc(sizeof(igloo_thread_type));

    thread->thread_id = igloo__next_thread_id++;
    thread->line = 0;
    thread->file = strdup("main.c");
    thread->sys_thread = pthread_self();
    thread->create_time = time(NULL);
    thread->name = strdup("Main Thread");

    igloo_avl_insert(igloo__threadtree, (void *)thread);

    _catch_signals();

    igloo__initialized = 1;
}

void igloo_thread_shutdown(void)
{
    if (igloo__initialized == 1) {
        igloo_thread_mutex_destroy(&igloo__library_mutex);
        igloo_thread_mutex_destroy(&igloo__threadtree_mutex);
#ifdef THREAD_DEBUG
        igloo_thread_mutex_destroy(&_mutextree_mutex);

        igloo_avl_tree_free(_mutextree, _free_mutex);
#endif
        igloo_avl_tree_free(igloo__threadtree, igloo__free_thread);
        igloo__threadtree = NULL;
    }

#ifdef THREAD_DEBUG
    igloo_log_close(_logid);
    igloo_log_shutdown();
#endif

}

/*
 * Signals should be handled by the main thread, nowhere else.
 * I'm using POSIX signal interface here, until someone tells me
 * that I should use signal/sigset instead
 *
 * This function only valid for non-Win32
 */
static void _block_signals(void)
{
#if !defined(_WIN32) && !defined(__ANDROID__)
        sigset_t ss;

        sigfillset(&ss);

        /* These ones we want */
        sigdelset(&ss, SIGKILL);
        sigdelset(&ss, SIGSTOP);
        sigdelset(&ss, SIGSEGV);
        sigdelset(&ss, SIGCHLD);
        sigdelset(&ss, SIGBUS);
        if (pthread_sigmask(SIG_BLOCK, &ss, NULL) != 0) {
#ifdef THREAD_DEBUG
                LOG_ERROR("Pthread_sigmask() failed for blocking signals");
#endif
        }
#endif
}

/*
 * Let the calling thread catch all the relevant signals
 *
 * This function only valid for non-Win32
 */
static void _catch_signals(void)
{
#if !defined(_WIN32) && !defined(__ANDROID__)
        sigset_t ss;

        sigemptyset(&ss);

        /* These ones should only be accepted by the signal handling thread (main thread) */
        sigaddset(&ss, SIGHUP);
        sigaddset(&ss, SIGCHLD);
        sigaddset(&ss, SIGINT);
        sigaddset(&ss, SIGPIPE);
        sigaddset(&ss, SIGTERM);

        if (pthread_sigmask(SIG_UNBLOCK, &ss, NULL) != 0) {
#ifdef THREAD_DEBUG
                LOG_ERROR("pthread_sigmask() failed for catching signals!");
#endif
        }
#endif
}


igloo_thread_type *igloo_thread_create_c(char *name, void *(*start_routine)(void *),
        void *arg, int detached, int line, char *file)
{
    igloo_thread_type *thread = NULL;
    thread_start_t *start = NULL;
    pthread_attr_t attr;

    thread = (igloo_thread_type *)calloc(1, sizeof(igloo_thread_type));
    do {
        if (thread == NULL)
            break;
        start = (thread_start_t *)calloc(1, sizeof(thread_start_t));
        if (start == NULL)
            break;
        if (pthread_attr_init (&attr) < 0)
            break;

        thread->line = line;
        thread->file = strdup(file);

        _mutex_lock (&igloo__threadtree_mutex);
        thread->thread_id = igloo__next_thread_id++;
        _mutex_unlock (&igloo__threadtree_mutex);

        thread->name = strdup(name);
        thread->create_time = time(NULL);

        start->start_routine = start_routine;
        start->arg = arg;
        start->thread = thread;

        pthread_attr_setstacksize (&attr, 512*1024);

#ifndef __ANDROID__
        pthread_attr_setinheritsched (&attr, PTHREAD_INHERIT_SCHED);
#endif

        if (detached)
        {
            pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
            thread->detached = 1;
        }

        if (pthread_create (&thread->sys_thread, &attr, igloo__start_routine, start) == 0)
        {
            pthread_attr_destroy (&attr);
            return thread;
        }
        else
            pthread_attr_destroy (&attr);
    }
    while (0);

#ifdef THREAD_DEBUG
    LOG_ERROR("Could not create new thread %s", name);
#endif
    if (start) free (start);
    if (thread) free (thread);
    return NULL;
}

/* _mutex_create
**
** creates a mutex
*/
static void _mutex_create(igloo_mutex_t *mutex)
{
#ifdef DEBUG_MUTEXES
    mutex->thread_id = igloo_MUTEX_STATE_NEVERLOCKED;
    mutex->line = -1;
#endif

    pthread_mutex_init(&mutex->sys_mutex, NULL);
}

void igloo_thread_mutex_create_c(igloo_mutex_t *mutex, int line, char *file)
{
    _mutex_create(mutex);

#ifdef DEBUG_MUTEXES
    _mutex_lock(&_mutextree_mutex);
    mutex->mutex_id = _next_mutex_id++;
    igloo_avl_insert(_mutextree, (void *)mutex);
    _mutex_unlock(&_mutextree_mutex);
#endif
}

void igloo_thread_mutex_destroy (igloo_mutex_t *mutex)
{
    pthread_mutex_destroy(&mutex->sys_mutex);

#ifdef DEBUG_MUTEXES
    _mutex_lock(&_mutextree_mutex);
    igloo_avl_delete(_mutextree, mutex, _free_mutex);
    _mutex_unlock(&_mutextree_mutex);
#endif
}

void igloo_thread_mutex_lock_c(igloo_mutex_t *mutex, int line, char *file)
{
#ifdef DEBUG_MUTEXES
    igloo_thread_type *th = igloo_thread_self();

    if (!th) LOG_WARN("No mt record for %u in lock [%s:%d]", igloo_thread_self(), file, line);

    LOG_DEBUG5("Locking %p (%s) on line %d in file %s by thread %d", mutex, mutex->name, line, file, th ? th->thread_id : -1);

# ifdef CHECK_MUTEXES
    /* Just a little sanity checking to make sure that we're locking
    ** mutexes correctly
    */

    if (th) {
        int locks = 0;
        igloo_avl_node *node;
        igloo_mutex_t *tmutex;

        _mutex_lock(&_mutextree_mutex);

        node = igloo_avl_get_first (_mutextree);

        while (node) {
            tmutex = (igloo_mutex_t *)node->key;

            if (tmutex->mutex_id == mutex->mutex_id) {
                if (tmutex->thread_id == th->thread_id) {
                    /* Deadlock, same thread can't lock the same mutex twice */
                    LOG_ERROR7("DEADLOCK AVOIDED (%d == %d) on mutex [%s] in file %s line %d by thread %d [%s]",
                         tmutex->thread_id, th->thread_id, mutex->name ? mutex->name : "undefined", file, line, th->thread_id, th->name);

                    _mutex_unlock(&_mutextree_mutex);
                    return;
                }
            } else if (tmutex->thread_id == th->thread_id) {
                /* Mutex locked by this thread (not this mutex) */
                locks++;
            }

            node = igloo_avl_get_next(node);
        }

        if (locks > 0) {
            /* Has already got a mutex locked */
            if (_multi_mutex.thread_id != th->thread_id) {
                /* Tries to lock two mutexes, but has not got the double mutex, norty boy! */
                LOG_WARN("(%d != %d) Thread %d [%s] tries to lock a second mutex [%s] in file %s line %d, without locking double mutex!",
                     _multi_mutex.thread_id, th->thread_id, th->thread_id, th->name, mutex->name ? mutex->name : "undefined", file, line);
            }
        }

        _mutex_unlock(&_mutextree_mutex);
    }
# endif /* CHECK_MUTEXES */

    _mutex_lock(mutex);

    _mutex_lock(&_mutextree_mutex);

    LOG_DEBUG2("Locked %p by thread %d", mutex, th ? th->thread_id : -1);
    mutex->line = line;
    if (th) {
        mutex->thread_id = th->thread_id;
    }

    _mutex_unlock(&_mutextree_mutex);
#else
    _mutex_lock(mutex);
#endif /* DEBUG_MUTEXES */
}

void igloo_thread_mutex_unlock_c(igloo_mutex_t *mutex, int line, char *file)
{
#ifdef DEBUG_MUTEXES
    igloo_thread_type *th = igloo_thread_self();

    if (!th) {
        LOG_ERROR3("No record for %u in unlock [%s:%d]", igloo_thread_self(), file, line);
    }

    LOG_DEBUG5("Unlocking %p (%s) on line %d in file %s by thread %d", mutex, mutex->name, line, file, th ? th->thread_id : -1);

    mutex->line = line;

# ifdef CHECK_MUTEXES
    if (th) {
        int locks = 0;
        igloo_avl_node *node;
        igloo_mutex_t *tmutex;

        _mutex_lock(&_mutextree_mutex);

        while (node) {
            tmutex = (igloo_mutex_t *)node->key;

            if (tmutex->mutex_id == mutex->mutex_id) {
                if (tmutex->thread_id != th->thread_id) {
                    LOG_ERROR7("ILLEGAL UNLOCK (%d != %d) on mutex [%s] in file %s line %d by thread %d [%s]", tmutex->thread_id, th->thread_id,
                         mutex->name ? mutex->name : "undefined", file, line, th->thread_id, th->name);
                    _mutex_unlock(&_mutextree_mutex);
                    return;
                }
            } else if (tmutex->thread_id == th->thread_id) {
                locks++;
            }

            node = igloo_avl_get_next (node);
        }

        if ((locks > 0) && (_multi_mutex.thread_id != th->thread_id)) {
            /* Don't have double mutex, has more than this mutex left */

            LOG_WARN("(%d != %d) Thread %d [%s] tries to unlock a mutex [%s] in file %s line %d, without owning double mutex!",
                 _multi_mutex.thread_id, th->thread_id, th->thread_id, th->name, mutex->name ? mutex->name : "undefined", file, line);
        }

        _mutex_unlock(&_mutextree_mutex);
    }
# endif  /* CHECK_MUTEXES */

    _mutex_unlock(mutex);

    _mutex_lock(&_mutextree_mutex);

    LOG_DEBUG2("Unlocked %p by thread %d", mutex, th ? th->thread_id : -1);
    mutex->line = -1;
    if (mutex->thread_id == th->thread_id) {
        mutex->thread_id = igloo_MUTEX_STATE_NOTLOCKED;
    }

    _mutex_unlock(&_mutextree_mutex);
#else
    _mutex_unlock(mutex);
#endif /* DEBUG_MUTEXES */
}

void igloo_thread_cond_create_c(igloo_cond_t *cond, int line, char *file)
{
    pthread_cond_init(&cond->sys_cond, NULL);
    pthread_mutex_init(&cond->cond_mutex, NULL);
}

void igloo_thread_cond_destroy(igloo_cond_t *cond)
{
    pthread_mutex_destroy(&cond->cond_mutex);
    pthread_cond_destroy(&cond->sys_cond);
}

void igloo_thread_cond_signal_c(igloo_cond_t *cond, int line, char *file)
{
    pthread_cond_signal(&cond->sys_cond);
}

void igloo_thread_cond_broadcast_c(igloo_cond_t *cond, int line, char *file)
{
    pthread_cond_broadcast(&cond->sys_cond);
}

void igloo_thread_cond_timedwait_c(igloo_cond_t *cond, int millis, int line, char *file)
{
    struct timespec time;

    time.tv_sec = millis/1000;
    time.tv_nsec = (millis - time.tv_sec*1000)*1000000;

    pthread_mutex_lock(&cond->cond_mutex);
    pthread_cond_timedwait(&cond->sys_cond, &cond->cond_mutex, &time);
    pthread_mutex_unlock(&cond->cond_mutex);
}

void igloo_thread_cond_wait_c(igloo_cond_t *cond, int line, char *file)
{
    pthread_mutex_lock(&cond->cond_mutex);
    pthread_cond_wait(&cond->sys_cond, &cond->cond_mutex);
    pthread_mutex_unlock(&cond->cond_mutex);
}

void igloo_thread_rwlock_create_c(igloo_rwlock_t *rwlock, int line, char *file)
{
    pthread_rwlock_init(&rwlock->sys_rwlock, NULL);
}

void igloo_thread_rwlock_destroy(igloo_rwlock_t *rwlock)
{
    pthread_rwlock_destroy(&rwlock->sys_rwlock);
}

void igloo_thread_rwlock_rlock_c(igloo_rwlock_t *rwlock, int line, char *file)
{
    pthread_rwlock_rdlock(&rwlock->sys_rwlock);
}

void igloo_thread_rwlock_wlock_c(igloo_rwlock_t *rwlock, int line, char *file)
{
    pthread_rwlock_wrlock(&rwlock->sys_rwlock);
}

void igloo_thread_rwlock_unlock_c(igloo_rwlock_t *rwlock, int line, char *file)
{
    pthread_rwlock_unlock(&rwlock->sys_rwlock);
}

void igloo_thread_exit_c(long val, int line, char *file)
{
    igloo_thread_type *th = igloo_thread_self();

#if defined(DEBUG_MUTEXES) && defined(CHECK_MUTEXES)
    if (th) {
        igloo_avl_node *node;
        igloo_mutex_t *tmutex;
        char name[40];

        _mutex_lock(&_mutextree_mutex);

        while (node) {
            tmutex = (igloo_mutex_t *)node->key;

            if (tmutex->thread_id == th->thread_id) {
                LOG_WARN("Thread %d [%s] exiting in file %s line %d, without unlocking mutex [%s]",
                     th->thread_id, th->name, file, line, mutex_to_string(tmutex, name));
            }

            node = igloo_avl_get_next (node);
        }

        _mutex_unlock(&_mutextree_mutex);
    }
#endif

    if (th && th->detached)
    {
#ifdef THREAD_DEBUG
        LOG_INFO4("Removing thread %d [%s] started at [%s:%d], reason: 'Thread Exited'", th->thread_id, th->name, th->file, th->line);
#endif

        _mutex_lock(&igloo__threadtree_mutex);
        igloo_avl_delete(igloo__threadtree, th, igloo__free_thread);
        _mutex_unlock(&igloo__threadtree_mutex);
    }

    pthread_exit ((void*)val);
}

/* sleep for a number of microseconds */
void igloo_thread_sleep(unsigned long len)
{
#ifdef _WIN32
    Sleep(len / 1000);
#else
# ifdef HAVE_NANOSLEEP
    struct timespec time_sleep;
    struct timespec time_remaining;
    int ret;

    time_sleep.tv_sec = len / 1000000;
    time_sleep.tv_nsec = (len % 1000000) * 1000;

    ret = nanosleep(&time_sleep, &time_remaining);
    while (ret != 0 && errno == EINTR) {
        time_sleep.tv_sec = time_remaining.tv_sec;
        time_sleep.tv_nsec = time_remaining.tv_nsec;

        ret = nanosleep(&time_sleep, &time_remaining);
    }
# else
    struct timeval tv;

    tv.tv_sec = len / 1000000;
    tv.tv_usec = (len % 1000000);

    select(0, NULL, NULL, NULL, &tv);
# endif
#endif
}

static void *igloo__start_routine(void *arg)
{
    thread_start_t *start = (thread_start_t *)arg;
    void *(*start_routine)(void *) = start->start_routine;
    void *real_arg = start->arg;
    igloo_thread_type *thread = start->thread;

    _block_signals();

    /* insert thread into thread tree here */
    _mutex_lock(&igloo__threadtree_mutex);
    thread->sys_thread = pthread_self();
    igloo_avl_insert(igloo__threadtree, (void *)thread);
    _mutex_unlock(&igloo__threadtree_mutex);

#ifdef THREAD_DEBUG
    LOG_INFO4("Added thread %d [%s] started at [%s:%d]", thread->thread_id, thread->name, thread->file, thread->line);
#endif

#ifndef __ANDROID__
    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
#endif

    free (start);

    (start_routine)(real_arg);

    if (thread->detached)
    {
        _mutex_lock (&igloo__threadtree_mutex);
        igloo_avl_delete (igloo__threadtree, thread, igloo__free_thread);
        _mutex_unlock (&igloo__threadtree_mutex);
    }

    return NULL;
}

igloo_thread_type *igloo_thread_self(void)
{
    igloo_avl_node *node;
    igloo_thread_type *th;
    pthread_t sys_thread = pthread_self();

    _mutex_lock(&igloo__threadtree_mutex);

    if (igloo__threadtree == NULL) {
#ifdef THREAD_DEBUG
        LOG_WARN("Thread tree is empty, this must be wrong!");
#endif
        _mutex_unlock(&igloo__threadtree_mutex);
        return NULL;
    }

    node = igloo_avl_get_first(igloo__threadtree);

    while (node) {
        th = (igloo_thread_type *)node->key;

        if (th && pthread_equal(sys_thread, th->sys_thread)) {
            _mutex_unlock(&igloo__threadtree_mutex);
            return th;
        }

        node = igloo_avl_get_next(node);
    }
    _mutex_unlock(&igloo__threadtree_mutex);


#ifdef THREAD_DEBUG
    LOG_ERROR("Nonexistant thread alive...");
#endif

    return NULL;
}

void igloo_thread_rename(const char *name)
{
    igloo_thread_type *th;

    th = igloo_thread_self();
    if (th->name) free(th->name);

    th->name = strdup(name);
}

static void _mutex_lock(igloo_mutex_t *mutex)
{
    pthread_mutex_lock(&mutex->sys_mutex);
}

static void _mutex_unlock(igloo_mutex_t *mutex)
{
    pthread_mutex_unlock(&mutex->sys_mutex);
}


void igloo_thread_library_lock(void)
{
    _mutex_lock(&igloo__library_mutex);
}

void igloo_thread_library_unlock(void)
{
    _mutex_unlock(&igloo__library_mutex);
}

void igloo_thread_join(igloo_thread_type *thread)
{
    void *ret;

    pthread_join(thread->sys_thread, &ret);
    _mutex_lock(&igloo__threadtree_mutex);
    igloo_avl_delete(igloo__threadtree, thread, igloo__free_thread);
    _mutex_unlock(&igloo__threadtree_mutex);
}

/* AVL tree functions */

#ifdef DEBUG_MUTEXES
static int _compare_mutexes(void *compare_arg, void *a, void *b)
{
    igloo_mutex_t *m1, *m2;

    m1 = (igloo_mutex_t *)a;
    m2 = (igloo_mutex_t *)b;

    if (m1->mutex_id > m2->mutex_id)
        return 1;
    if (m1->mutex_id < m2->mutex_id)
        return -1;
    return 0;
}
#endif

static int igloo__compare_threads(void *compare_arg, void *a, void *b)
{
    igloo_thread_type *t1, *t2;

    t1 = (igloo_thread_type *)a;
    t2 = (igloo_thread_type *)b;

    if (t1->thread_id > t2->thread_id)
        return 1;
    if (t1->thread_id < t2->thread_id)
        return -1;
    return 0;
}

#ifdef DEBUG_MUTEXES
static int _free_mutex(void *key)
{
    igloo_mutex_t *m;

    m = (igloo_mutex_t *)key;

    if (m && m->file) {
        free(m->file);
        m->file = NULL;
    }

    /* all mutexes are static.  don't need to free them */

    return 1;
}
#endif

static int igloo__free_thread(void *key)
{
    igloo_thread_type *t;

    t = (igloo_thread_type *)key;

    if (t->file)
        free(t->file);
    if (t->name)
        free(t->name);

    free(t);

    return 1;
}


#ifdef HAVE_PTHREAD_SPIN_LOCK
void igloo_thread_spin_create (igloo_spin_t *spin)
{
    int x = pthread_spin_init (&spin->lock, PTHREAD_PROCESS_PRIVATE);
    if (x)
        abort();
}

void igloo_thread_spin_destroy (igloo_spin_t *spin)
{
    pthread_spin_destroy (&spin->lock);
}

void igloo_thread_spin_lock (igloo_spin_t *spin)
{
    int x = pthread_spin_lock (&spin->lock);
    if (x != 0)
        abort();
}

void igloo_thread_spin_unlock (igloo_spin_t *spin)
{
    pthread_spin_unlock (&spin->lock);
}
#endif

