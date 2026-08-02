/* See pthread.h: exactly one schedulable context ever exists here, so
 * these are honest single-threaded implementations, not fakes -- a
 * recursion counter really is mutual exclusion when there is no second
 * thread to exclude. */
#include <pthread.h>
#include <sched.h>
#include <stddef.h>
#include <errno.h>

extern int errno;

int pthread_mutexattr_init(pthread_mutexattr_t *attr) { attr->type = PTHREAD_MUTEX_NORMAL; return 0; }
int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type) { attr->type = type; return 0; }
int pthread_mutexattr_destroy(pthread_mutexattr_t *attr) { (void)attr; return 0; }

int pthread_mutex_init(pthread_mutex_t *m, const pthread_mutexattr_t *attr) { (void)attr; m->locked = 0; return 0; }
int pthread_mutex_lock(pthread_mutex_t *m) { m->locked++; return 0; }
int pthread_mutex_trylock(pthread_mutex_t *m) { m->locked++; return 0; }
int pthread_mutex_unlock(pthread_mutex_t *m) { if (m->locked > 0) { m->locked--; } return 0; }
int pthread_mutex_destroy(pthread_mutex_t *m) { (void)m; return 0; }

int pthread_cond_init(pthread_cond_t *cv, const pthread_condattr_t *attr) { (void)attr; cv->unused = 0; return 0; }
int pthread_cond_signal(pthread_cond_t *cv) { (void)cv; return 0; }
int pthread_cond_broadcast(pthread_cond_t *cv) { (void)cv; return 0; }

/* Waiting for a condition only another thread could ever signal would
 * deadlock forever in a single-threaded environment -- there is no
 * second thread to wake us. Returning immediately is the only sane
 * choice here; correct only because nothing in this build actually
 * depends on a real blocking wait (LLVM_ENABLE_THREADS=OFF). TODO:
 * revisit if something ever genuinely needs to block on a condition. */
int pthread_cond_wait(pthread_cond_t *cv, pthread_mutex_t *m) { (void)cv; (void)m; return 0; }
int pthread_cond_timedwait(pthread_cond_t *cv, pthread_mutex_t *m, const struct timespec *ts) { (void)cv; (void)m; (void)ts; return 0; }
int pthread_cond_destroy(pthread_cond_t *cv) { (void)cv; return 0; }

int
pthread_once(pthread_once_t *flag, void (*init_routine)(void))
{
	if (!*flag) {
		*flag = 1;
		init_routine();
	}
	return 0;
}

#define PTHREAD_KEY_MAX 64
static void *g_tsd[PTHREAD_KEY_MAX];
static int g_next_key;

int
pthread_key_create(pthread_key_t *key, void (*destructor)(void *))
{
	(void)destructor; /* never called: the one context never "exits" */
	if (g_next_key >= PTHREAD_KEY_MAX) {
		return EAGAIN;
	}
	*key = g_next_key++;
	return 0;
}

int pthread_key_delete(pthread_key_t key) { (void)key; return 0; }
void *pthread_getspecific(pthread_key_t key) { return (key >= 0 && key < PTHREAD_KEY_MAX) ? g_tsd[key] : NULL; }
int pthread_setspecific(pthread_key_t key, const void *value)
{
	if (key < 0 || key >= PTHREAD_KEY_MAX) {
		return EINVAL;
	}
	g_tsd[key] = (void *)value;
	return 0;
}

pthread_t pthread_self(void) { return 1; }
int pthread_equal(pthread_t a, pthread_t b) { return a == b; }
mach_port_t pthread_mach_thread_np(pthread_t thread) { (void)thread; return 1; }

int pthread_rwlock_init(pthread_rwlock_t *rwlock, const pthread_rwlockattr_t *attr) { (void)attr; rwlock->locked = 0; return 0; }
int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock) { rwlock->locked++; return 0; }
int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock) { rwlock->locked++; return 0; }
int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock) { rwlock->locked++; return 0; }
int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock) { rwlock->locked++; return 0; }
int pthread_rwlock_unlock(pthread_rwlock_t *rwlock) { if (rwlock->locked > 0) { rwlock->locked--; } return 0; }
int pthread_rwlock_destroy(pthread_rwlock_t *rwlock) { (void)rwlock; return 0; }

int pthread_attr_init(pthread_attr_t *attr) { attr->stacksize = 0; attr->detachstate = 0; return 0; }
int pthread_attr_destroy(pthread_attr_t *attr) { (void)attr; return 0; }
int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize) { attr->stacksize = stacksize; return 0; }
int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize) { *stacksize = attr->stacksize; return 0; }

int
pthread_create(pthread_t *thread, const pthread_attr_t *attr,
    void *(*start_routine)(void *), void *arg)
{
	(void)thread; (void)attr; (void)start_routine; (void)arg;
	return EAGAIN; /* no real kernel thread-creation support -- see pthread.h */
}

int pthread_join(pthread_t thread, void **retval) { (void)thread; if (retval) { *retval = NULL; } return 0; }
int pthread_detach(pthread_t thread) { (void)thread; return 0; }

int sched_yield(void) { return 0; }
