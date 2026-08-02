/* There is exactly one schedulable context in this environment (no real
 * kernel thread-creation support -- see docs/architecture.md's "no
 * dyld/Libsystem" decision, same single-threaded philosophy). libc++ is
 * built with real threading enabled anyway (LIBCXX_ENABLE_THREADS=ON)
 * because LLVM's own Support library (ThreadPool.h, BalancedPartitioning.h)
 * unconditionally names std::mutex/std::condition_variable/std::future
 * regardless of LLVM_ENABLE_THREADS -- those types need a real pthread
 * ABI to exist, even though nothing here ever actually runs two threads
 * at once. Every primitive below is honestly correct for that single
 * schedulable context: mutexes track a recursion count instead of doing
 * real mutual exclusion (there is no second thread to exclude), and
 * pthread_create() honestly fails since we cannot create one. */
#ifndef _PTHREAD_H_
#define _PTHREAD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

typedef struct {
	int type;
} pthread_mutexattr_t;

typedef struct {
	int locked;
} pthread_mutex_t;

#define PTHREAD_MUTEX_INITIALIZER { 0 }
#define PTHREAD_MUTEX_NORMAL     0
#define PTHREAD_MUTEX_RECURSIVE  1
#define PTHREAD_MUTEX_DEFAULT    PTHREAD_MUTEX_NORMAL

typedef struct {
	int unused;
} pthread_condattr_t;

typedef struct {
	int unused;
} pthread_cond_t;

#define PTHREAD_COND_INITIALIZER { 0 }

typedef int pthread_once_t;
#define PTHREAD_ONCE_INIT 0

typedef int pthread_key_t;
typedef unsigned long pthread_t;
typedef struct {
	size_t stacksize;
	int detachstate;
} pthread_attr_t;

typedef struct {
	int locked;
} pthread_rwlock_t;
typedef struct {
	int unused;
} pthread_rwlockattr_t;

#define PTHREAD_RWLOCK_INITIALIZER { 0 }

int pthread_rwlock_init(pthread_rwlock_t *rwlock, const pthread_rwlockattr_t *attr);
int pthread_rwlock_rdlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_wrlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_unlock(pthread_rwlock_t *rwlock);
int pthread_rwlock_destroy(pthread_rwlock_t *rwlock);

/* mach_port_t really belongs in <mach/port.h>, but real Darwin's own
 * <pthread.h> declares pthread_mach_thread_np() (and pulls in
 * mach_port_t for it) directly, so callers that only include <pthread.h>
 * -- like libc++abi's guard implementation -- still see it. Matched here
 * for the same reason. */
typedef unsigned int mach_port_t;
mach_port_t pthread_mach_thread_np(pthread_t thread);

int pthread_mutexattr_init(pthread_mutexattr_t *attr);
int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);
int pthread_mutexattr_destroy(pthread_mutexattr_t *attr);

int pthread_mutex_init(pthread_mutex_t *m, const pthread_mutexattr_t *attr);
int pthread_mutex_lock(pthread_mutex_t *m);
int pthread_mutex_trylock(pthread_mutex_t *m);
int pthread_mutex_unlock(pthread_mutex_t *m);
int pthread_mutex_destroy(pthread_mutex_t *m);

int pthread_cond_init(pthread_cond_t *cv, const pthread_condattr_t *attr);
int pthread_cond_signal(pthread_cond_t *cv);
int pthread_cond_broadcast(pthread_cond_t *cv);
int pthread_cond_wait(pthread_cond_t *cv, pthread_mutex_t *m);
struct timespec;
int pthread_cond_timedwait(pthread_cond_t *cv, pthread_mutex_t *m, const struct timespec *ts);
int pthread_cond_destroy(pthread_cond_t *cv);

int pthread_once(pthread_once_t *flag, void (*init_routine)(void));

int pthread_key_create(pthread_key_t *key, void (*destructor)(void *));
int pthread_key_delete(pthread_key_t key);
void *pthread_getspecific(pthread_key_t key);
int pthread_setspecific(pthread_key_t key, const void *value);

pthread_t pthread_self(void);
int pthread_attr_init(pthread_attr_t *attr);
int pthread_attr_destroy(pthread_attr_t *attr);
int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize);
int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize);
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
    void *(*start_routine)(void *), void *arg);
int pthread_join(pthread_t thread, void **retval);
int pthread_detach(pthread_t thread);
int pthread_equal(pthread_t a, pthread_t b);

#ifdef __cplusplus
}
#endif

#endif /* _PTHREAD_H_ */
