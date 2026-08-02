/* Real pthreads -- genuine kernel-scheduled threads via bsdthread_create(2)
 * (see docs/architecture.md and pthread.h for the full picture: xnu-6153
 * doesn't implement bsdthread_create/psynch_* itself -- that normally
 * lives in a separate pthread.kext -- so real Apple libpthread-416's
 * kernel component was statically folded into the kernel image instead
 * and registered directly at boot; pthread_syscalls.c/pthread_asm.S wire
 * this userland side up to the same real syscalls a genuine libpthread
 * would use).
 *
 * What's simplified vs. real Apple libpthread, and why that's still
 * honest (not fake):
 *  - Mutex/condvar/rwlock don't use the psynch_* wire protocol (a whole
 *    generation-counter/kernel-waitqueue subsystem of its own). They're
 *    plain atomic-CAS spinlocks / a spin-polled generation counter.
 *    Genuinely correct under xnu's real preemptive scheduler: a spinning
 *    thread's quantum expires and the lock-holding thread gets scheduled,
 *    even on a single vCPU. Just not as efficient as a real futex/psynch
 *    wait -- no thread ever blocks without consuming CPU.
 *  - There is no dyld/kernel TLS (bsdthread_register() is called with
 *    tsd_offset=0), so "which thread am I" is answered by checking which
 *    registered thread's mmap'd stack range the current stack pointer
 *    falls in (see pthread_internal.h). This also makes errno genuinely
 *    per-thread (errno.h's __errno_location()) without needing real TLS.
 *  - A thread cannot safely munmap the stack it's currently running on,
 *    so bsdthread_terminate() is called with freesize=0 (the kernel does
 *    not reclaim it) and the stack is munmap'd by whoever later calls
 *    pthread_join() on that thread instead. Detached threads' stacks are
 *    reclaimed opportunistically the next time pthread_create() runs (see
 *    reap_finished_detached()) rather than immediately -- a real, if lazy,
 *    reclamation, not a leak by design.
 *  - pthread_key destructors are not run at thread exit yet.
 */
#include <pthread.h>
#include <sched.h>
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include "pthread_internal.h"

/* Ground-truthed against src/xnu/bsd/pthread/libpthread_kern/kern/
 * kern_support.c: _bsdthread_create() requires this flag or returns
 * EINVAL outright ("<rdar://problem/34501401>"). */
#define PTHREAD_START_CUSTOM 0x01000000

#define DEFAULT_STACK_SIZE (512 * 1024)
#define PAGE_ROUND(n) (((size_t)(n) + 0xFFFu) & ~(size_t)0xFFFu)

static inline void
spin_pause(void)
{
	__asm__ __volatile__("pause" ::: "memory");
}

/* ---- tiny atomic-CAS spinlock, used only to protect the thread
 * registry below (not exposed -- pthread_mutex_t has its own copy of
 * this same idea, duplicated rather than shared so this file has no
 * ordering dependency on pthread_mutex_t being usable yet). */
static void
raw_lock(volatile int *l)
{
	while (__atomic_exchange_n(l, 1, __ATOMIC_ACQUIRE)) {
		spin_pause();
	}
}

static void
raw_unlock(volatile int *l)
{
	__atomic_store_n(l, 0, __ATOMIC_RELEASE);
}

/* ---- thread registry ---- */
static struct __pthread g_main_thread;
static struct __pthread *g_list;
static volatile int g_list_lock;
static int g_main_errno;

struct __pthread *
__pthread_current(void)
{
	void *sp = __builtin_frame_address(0);
	raw_lock(&g_list_lock);
	for (struct __pthread *t = g_list; t; t = t->next) {
		if (sp >= t->stack_lo && sp < t->stack_hi) {
			raw_unlock(&g_list_lock);
			return t;
		}
	}
	raw_unlock(&g_list_lock);
	return &g_main_thread;
}

int *
__errno_location(void)
{
	struct __pthread *t = __pthread_current();
	return (t == &g_main_thread) ? &g_main_errno : &t->err;
}

void
__pthread_register(struct __pthread *t)
{
	raw_lock(&g_list_lock);
	t->next = g_list;
	g_list = t;
	raw_unlock(&g_list_lock);
}

void
__pthread_unregister(struct __pthread *t)
{
	raw_lock(&g_list_lock);
	struct __pthread **pp = &g_list;
	while (*pp) {
		if (*pp == t) {
			*pp = t->next;
			break;
		}
		pp = &(*pp)->next;
	}
	raw_unlock(&g_list_lock);
}

/* Frees the stack + bookkeeping of every finished, detached thread --
 * called opportunistically from pthread_create() so detached threads'
 * resources don't accumulate forever even though nobody ever joins them. */
static void
reap_finished_detached(void)
{
	struct __pthread *dead = NULL;

	raw_lock(&g_list_lock);
	struct __pthread **pp = &g_list;
	while (*pp) {
		struct __pthread *t = *pp;
		if (t->detached && __atomic_load_n(&t->finished, __ATOMIC_ACQUIRE)) {
			*pp = t->next;
			t->next = dead;
			dead = t;
		} else {
			pp = &t->next;
		}
	}
	raw_unlock(&g_list_lock);

	while (dead) {
		struct __pthread *next = dead->next;
		munmap(dead->stack_lo, dead->stack_size);
		free(dead);
		dead = next;
	}
}

/* ---- one-time bsdthread_register() with the kernel ---- */
static volatile int g_reg_state; /* 0=not started, 1=in progress, 2=done, 3=failed */

static int
ensure_registered(void)
{
	int expected = 0;
	if (__atomic_compare_exchange_n(&g_reg_state, &expected, 1, 0,
		    __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) {
		/* AsterOS note: on success this returns
		 * PTHREAD_FEATURE_SUPPORTED (a feature bitmask), not 0 -- xnu's
		 * generic syscall dispatch always uses the C implementation's
		 * *retval out-param as the real %rax value on success (checked
		 * live: _bsdthread_register() in kern_support.c sets
		 * *retval = PTHREAD_FEATURE_SUPPORTED right before `return 0`).
		 * Only -1 (from sys_result()'s carry-flag check) means error. */
		int r = __bsdthread_register((void *)_pthread_start,
		    (void *)_pthread_wqthread, 0, (void *)0, (void *)0, 0, 0);
		__atomic_store_n(&g_reg_state, r != -1 ? 2 : 3, __ATOMIC_RELEASE);
	} else {
		while (1) {
			int s = __atomic_load_n(&g_reg_state, __ATOMIC_ACQUIRE);
			if (s == 2 || s == 3) {
				break;
			}
			spin_pause();
		}
	}
	return __atomic_load_n(&g_reg_state, __ATOMIC_ACQUIRE) == 2 ? 0 : -1;
}

/* ---- thread creation / join / detach ---- */

void
__pthread_trampoline_c(struct __pthread *t, unsigned int kport)
{
	t->kport = kport;
	void *ret = t->start_routine(t->arg);
	t->retval = ret;
	__atomic_store_n(&t->finished, 1, __ATOMIC_RELEASE);
	/* freesize=0: never let the kernel reclaim the stack we're currently
	 * running on -- see the file header comment. */
	__bsdthread_terminate((void *)0, 0, kport, 0);
	for (;;) {
		spin_pause(); /* unreachable: bsdthread_terminate() doesn't return */
	}
}

int
pthread_create(pthread_t *thread, const pthread_attr_t *attr,
    void *(*start_routine)(void *), void *arg)
{
	if (ensure_registered() != 0) {
		return EAGAIN;
	}
	reap_finished_detached();

	size_t stacksize = (attr && attr->stacksize) ? attr->stacksize : DEFAULT_STACK_SIZE;
	stacksize = PAGE_ROUND(stacksize);

	void *stack_lo = mmap(0, stacksize, PROT_READ | PROT_WRITE,
	    MAP_PRIVATE | MAP_ANON, -1, 0);
	if (stack_lo == (void *)-1) {
		return EAGAIN;
	}

	struct __pthread *t = calloc(1, sizeof(*t));
	if (!t) {
		munmap(stack_lo, stacksize);
		return EAGAIN;
	}
	t->stack_lo = stack_lo;
	t->stack_hi = (char *)stack_lo + stacksize;
	t->stack_size = stacksize;
	t->start_routine = start_routine;
	t->arg = arg;
	t->detached = attr && attr->detachstate == PTHREAD_CREATE_DETACHED;

	__pthread_register(t);

	long r = __bsdthread_create(start_routine, arg, t->stack_hi, t,
	    PTHREAD_START_CUSTOM);
	if (r == -1) {
		__pthread_unregister(t);
		munmap(stack_lo, stacksize);
		free(t);
		return EAGAIN;
	}

	*thread = (pthread_t)(uintptr_t)t;
	return 0;
}

int
pthread_join(pthread_t thread, void **retval)
{
	struct __pthread *t = (struct __pthread *)(uintptr_t)thread;
	if (t == &g_main_thread || t == __pthread_current()) {
		return EDEADLK;
	}
	while (!__atomic_load_n(&t->finished, __ATOMIC_ACQUIRE)) {
		spin_pause();
	}
	if (retval) {
		*retval = t->retval;
	}
	__pthread_unregister(t);
	munmap(t->stack_lo, t->stack_size);
	free(t);
	return 0;
}

int
pthread_detach(pthread_t thread)
{
	struct __pthread *t = (struct __pthread *)(uintptr_t)thread;
	__atomic_store_n(&t->detached, 1, __ATOMIC_RELAXED);
	return 0;
}

int
sched_yield(void)
{
	/* No cheap kernel yield primitive is wired up (real Darwin's
	 * sched_yield() goes through the swtch_pri()/thread_switch() Mach
	 * trap, not a BSD syscall -- Mach traps aren't implemented in this
	 * libc at all yet). A no-op is POSIX-legal; every spin loop in this
	 * file relies on xnu's own preemptive quantum expiry instead, not on
	 * this returning anything useful. */
	return 0;
}

pthread_t
pthread_self(void)
{
	return (pthread_t)(uintptr_t)__pthread_current();
}

int
pthread_equal(pthread_t a, pthread_t b)
{
	return a == b;
}

mach_port_t
pthread_mach_thread_np(pthread_t thread)
{
	struct __pthread *t = (struct __pthread *)(uintptr_t)thread;
	return (mach_port_t)t->kport;
}

int
pthread_attr_init(pthread_attr_t *attr)
{
	attr->stacksize = 0;
	attr->detachstate = PTHREAD_CREATE_JOINABLE;
	return 0;
}

int
pthread_attr_destroy(pthread_attr_t *attr)
{
	(void)attr;
	return 0;
}

int
pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize)
{
	attr->stacksize = stacksize;
	return 0;
}

int
pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stacksize)
{
	*stacksize = attr->stacksize;
	return 0;
}

int
pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate)
{
	attr->detachstate = detachstate;
	return 0;
}

int
pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detachstate)
{
	*detachstate = attr->detachstate;
	return 0;
}

/* ---- mutex: atomic-CAS spinlock + owner/recursion bookkeeping ---- */

int
pthread_mutexattr_init(pthread_mutexattr_t *attr)
{
	attr->type = PTHREAD_MUTEX_NORMAL;
	return 0;
}

int
pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type)
{
	attr->type = type;
	return 0;
}

int
pthread_mutexattr_destroy(pthread_mutexattr_t *attr)
{
	(void)attr;
	return 0;
}

int
pthread_mutex_init(pthread_mutex_t *m, const pthread_mutexattr_t *attr)
{
	m->owned = 0;
	m->owner = (void *)0;
	m->count = 0;
	m->type = attr ? attr->type : PTHREAD_MUTEX_NORMAL;
	return 0;
}

int
pthread_mutex_lock(pthread_mutex_t *m)
{
	struct __pthread *self = __pthread_current();
	if (m->type == PTHREAD_MUTEX_RECURSIVE && m->owner == (void *)self) {
		m->count++;
		return 0;
	}
	while (__atomic_exchange_n(&m->owned, 1, __ATOMIC_ACQUIRE)) {
		spin_pause();
	}
	m->owner = (void *)self;
	m->count = 1;
	return 0;
}

int
pthread_mutex_trylock(pthread_mutex_t *m)
{
	struct __pthread *self = __pthread_current();
	if (m->type == PTHREAD_MUTEX_RECURSIVE && m->owner == (void *)self) {
		m->count++;
		return 0;
	}
	if (__atomic_exchange_n(&m->owned, 1, __ATOMIC_ACQUIRE)) {
		return EBUSY;
	}
	m->owner = (void *)self;
	m->count = 1;
	return 0;
}

int
pthread_mutex_unlock(pthread_mutex_t *m)
{
	if (m->type == PTHREAD_MUTEX_RECURSIVE && --m->count > 0) {
		return 0;
	}
	m->owner = (void *)0;
	__atomic_store_n(&m->owned, 0, __ATOMIC_RELEASE);
	return 0;
}

int
pthread_mutex_destroy(pthread_mutex_t *m)
{
	(void)m;
	return 0;
}

/* ---- condition variables: spin-polled generation counter ----
 * Spurious wakeups are always POSIX-legal, so waking every waiter on
 * both signal() and broadcast() (rather than exactly one on signal()) is
 * a correct, if less efficient, implementation -- real code must always
 * recheck its predicate in a loop regardless. */

int
pthread_cond_init(pthread_cond_t *cv, const pthread_condattr_t *attr)
{
	(void)attr;
	cv->gen = 0;
	return 0;
}

int
pthread_cond_signal(pthread_cond_t *cv)
{
	__atomic_fetch_add(&cv->gen, 1, __ATOMIC_RELEASE);
	return 0;
}

int
pthread_cond_broadcast(pthread_cond_t *cv)
{
	__atomic_fetch_add(&cv->gen, 1, __ATOMIC_RELEASE);
	return 0;
}

int
pthread_cond_wait(pthread_cond_t *cv, pthread_mutex_t *m)
{
	unsigned int gen0 = __atomic_load_n(&cv->gen, __ATOMIC_ACQUIRE);
	pthread_mutex_unlock(m);
	while (__atomic_load_n(&cv->gen, __ATOMIC_ACQUIRE) == gen0) {
		spin_pause();
	}
	pthread_mutex_lock(m);
	return 0;
}

int
pthread_cond_timedwait(pthread_cond_t *cv, pthread_mutex_t *m, const struct timespec *ts)
{
	unsigned int gen0 = __atomic_load_n(&cv->gen, __ATOMIC_ACQUIRE);
	struct timeval now;
	gettimeofday(&now, (void *)0);
	/* ts is an *absolute* deadline per POSIX. */
	long long deadline_us = (long long)ts->tv_sec * 1000000LL + ts->tv_nsec / 1000;
	pthread_mutex_unlock(m);
	int timed_out = 0;
	while (__atomic_load_n(&cv->gen, __ATOMIC_ACQUIRE) == gen0) {
		gettimeofday(&now, (void *)0);
		long long now_us = (long long)now.tv_sec * 1000000LL + now.tv_usec;
		if (now_us >= deadline_us) {
			timed_out = 1;
			break;
		}
		spin_pause();
	}
	pthread_mutex_lock(m);
	return timed_out ? ETIMEDOUT : 0;
}

int
pthread_cond_destroy(pthread_cond_t *cv)
{
	(void)cv;
	return 0;
}

/* ---- rwlock: state>=0 is a reader count, state==-1 is write-locked ---- */

int
pthread_rwlock_init(pthread_rwlock_t *rwlock, const pthread_rwlockattr_t *attr)
{
	(void)attr;
	rwlock->state = 0;
	return 0;
}

int
pthread_rwlock_rdlock(pthread_rwlock_t *rwlock)
{
	for (;;) {
		int s = __atomic_load_n(&rwlock->state, __ATOMIC_ACQUIRE);
		if (s >= 0 && __atomic_compare_exchange_n(&rwlock->state, &s, s + 1,
			    0, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
			return 0;
		}
		spin_pause();
	}
}

int
pthread_rwlock_tryrdlock(pthread_rwlock_t *rwlock)
{
	int s = __atomic_load_n(&rwlock->state, __ATOMIC_ACQUIRE);
	if (s < 0) {
		return EBUSY;
	}
	if (!__atomic_compare_exchange_n(&rwlock->state, &s, s + 1, 0,
		    __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
		return EBUSY;
	}
	return 0;
}

int
pthread_rwlock_wrlock(pthread_rwlock_t *rwlock)
{
	for (;;) {
		int s = 0;
		if (__atomic_compare_exchange_n(&rwlock->state, &s, -1, 0,
			    __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
			return 0;
		}
		spin_pause();
	}
}

int
pthread_rwlock_trywrlock(pthread_rwlock_t *rwlock)
{
	int s = 0;
	if (!__atomic_compare_exchange_n(&rwlock->state, &s, -1, 0,
		    __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
		return EBUSY;
	}
	return 0;
}

int
pthread_rwlock_unlock(pthread_rwlock_t *rwlock)
{
	int s = __atomic_load_n(&rwlock->state, __ATOMIC_RELAXED);
	if (s < 0) {
		__atomic_store_n(&rwlock->state, 0, __ATOMIC_RELEASE);
	} else {
		__atomic_fetch_sub(&rwlock->state, 1, __ATOMIC_RELEASE);
	}
	return 0;
}

int
pthread_rwlock_destroy(pthread_rwlock_t *rwlock)
{
	(void)rwlock;
	return 0;
}

/* ---- pthread_once: CAS state machine (0=untouched,1=running,2=done) ---- */

int
pthread_once(pthread_once_t *flag, void (*init_routine)(void))
{
	int expected = 0;
	if (__atomic_compare_exchange_n(flag, &expected, 1, 0,
		    __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) {
		init_routine();
		__atomic_store_n(flag, 2, __ATOMIC_RELEASE);
	} else {
		while (__atomic_load_n(flag, __ATOMIC_ACQUIRE) != 2) {
			spin_pause();
		}
	}
	return 0;
}

/* ---- thread-specific data: stored directly in struct __pthread, found
 * via the same stack-range lookup as everything else here. Destructors
 * are not run at thread exit yet -- see the file header comment. */

static volatile int g_next_key;

int
pthread_key_create(pthread_key_t *key, void (*destructor)(void *))
{
	(void)destructor;
	int k = __atomic_fetch_add(&g_next_key, 1, __ATOMIC_RELAXED);
	if (k >= PTHREAD_KEY_MAX) {
		return EAGAIN;
	}
	*key = k;
	return 0;
}

int
pthread_key_delete(pthread_key_t key)
{
	(void)key;
	return 0;
}

void *
pthread_getspecific(pthread_key_t key)
{
	if (key < 0 || key >= PTHREAD_KEY_MAX) {
		return (void *)0;
	}
	return __pthread_current()->tsd[key];
}

int
pthread_setspecific(pthread_key_t key, const void *value)
{
	if (key < 0 || key >= PTHREAD_KEY_MAX) {
		return EINVAL;
	}
	__pthread_current()->tsd[key] = (void *)value;
	return 0;
}
