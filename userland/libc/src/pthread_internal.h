/* Internal bookkeeping for the real pthread implementation (pthread.c).
 * Not installed as a public header.
 *
 * Design constraints that shaped this file:
 *  - No dyld/kernel TLS is wired up (bsdthread_register() is called with
 *    tsd_offset=0), so there is no %fs-based "who am I" register. Instead
 *    every live thread is kept on a spinlock-protected linked list
 *    recording its stack's [lo,hi) bounds, and "which thread is this"
 *    is answered by checking which range the current stack pointer falls
 *    in (pthread_current()). The main thread (never bsdthread_create'd,
 *    so it has no entry) falls through to a static sentinel.
 *  - No Mach semaphore/psynch wiring exists here (both are real
 *    subsystems of their own, out of scope for this pass) -- mutexes are
 *    plain atomic-CAS spinlocks and condition variables are a spin-polled
 *    generation counter. Both are genuinely correct under xnu's real
 *    preemptive scheduler (a spinning thread's quantum expires and the
 *    lock/generation-owning thread gets scheduled -- this holds even on
 *    a single vCPU), just not as efficient as a real futex/psynch wait.
 *    Documented here rather than hidden.
 *  - errno must be made genuinely per-thread once real concurrent
 *    threads exist (see errno.h's __errno_location() macro) -- it reuses
 *    this same stack-range lookup.
 */
#ifndef DARWINBUILD_PTHREAD_INTERNAL_H
#define DARWINBUILD_PTHREAD_INTERNAL_H

#include <pthread.h>
#include <stddef.h>
#include <stdint.h>

#define PTHREAD_KEY_MAX 64

struct __pthread {
	struct __pthread *next;
	void *stack_lo;         /* mmap base, for munmap on join/detach */
	void *stack_hi;         /* stack_lo + stack_size */
	size_t stack_size;
	void *(*start_routine)(void *);
	void *arg;
	void *retval;
	int detached;            /* set at create time or by pthread_detach() */
	volatile int finished;   /* set by the trampoline just before it exits */
	int err;                 /* this thread's errno */
	unsigned int kport;      /* mach port name for this kernel thread */
	void *tsd[PTHREAD_KEY_MAX];
};

/* Returns the current thread's bookkeeping struct: a real entry from the
 * registry if the current stack pointer falls inside one, otherwise the
 * static main-thread sentinel. Never returns NULL. */
struct __pthread *__pthread_current(void);

/* Registry management, used by pthread_create()/join()/detach(). */
void __pthread_register(struct __pthread *t);
void __pthread_unregister(struct __pthread *t);

/* Real bsdthread_create(2)/bsdthread_register(2)/bsdthread_terminate(2)
 * wrappers -- see pthread_syscalls.c. */
int __bsdthread_register(void *threadstart, void *wqthread, uint32_t flags,
    void *stack_addr_hint, void *targetconc_ptr, uint32_t dispatchqueue_offset,
    uint32_t tsd_offset);
long __bsdthread_create(void *(*func)(void *), void *func_arg, void *stack,
    void *pthread, unsigned int flags);
int __bsdthread_terminate(void *stackaddr, size_t freesize, unsigned int port,
    unsigned int sem);

/* The kernel jumps here (see kern_support.c's _bsdthread_create register
 * setup) for every newly created thread: rdi=user_pthread, rsi=kport,
 * rdx=user_func, rcx=user_funcarg, r8=user_stack, r9=flags. Defined in
 * pthread_asm.S; calls into __pthread_trampoline_c below. */
void _pthread_start(void);

/* wqthread entry point bsdthread_register() must supply -- this project
 * never issues workq_open()/workq_kernreturn() (see TODO.md's libpthread
 * notes: this xnu's own pthread_workqueue.c handles GCD-style workqueue
 * threads entirely in-kernel and never calls back out through
 * pthread_functions for it), so the kernel never actually jumps here;
 * bsdthread_register() just requires a non-NULL pointer. */
void _pthread_wqthread(void);

/* C landing pad for _pthread_start, called with the stack pointer already
 * switched to the new thread's own stack. Runs start_routine, stores the
 * result, marks the thread finished, and terminates it -- never returns. */
void __pthread_trampoline_c(struct __pthread *t, unsigned int kport)
    __attribute__((noreturn));

#endif /* DARWINBUILD_PTHREAD_INTERNAL_H */
