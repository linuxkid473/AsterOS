/* Copyright (c) 2026 Vihaan Nathan
 *
 * v1-scoped GCD: dispatch_queue_t (serial + concurrent), dispatch_once,
 * dispatch_semaphore_t, dispatch_group_t, dispatch_after. Signatures match
 * real libdispatch's public dispatch.h so unmodified client code compiles;
 * the scheduler behind them (dispatch_internal.h) is our own, built on
 * real kernel-scheduled pthreads rather than xnu's real workqueue/kevent
 * machinery -- see docs/architecture.md's libdispatch scope section for
 * what's deliberately out of v1 (dispatch_source, dispatch_io, real QoS).
 *
 * Every block-taking entry point has an `_f` (function pointer + context)
 * twin, same as real libdispatch -- the scheduler itself only ever deals
 * in `_f` work items; the block-taking wrappers (dispatch_*.c) just
 * Block_copy the block and hand it through as the context.
 */
#ifndef __DISPATCH_DISPATCH_H__
#define __DISPATCH_DISPATCH_H__

#include <stdint.h>
#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DISPATCH_EXPORT extern

typedef void (^dispatch_block_t)(void);
typedef void (*dispatch_function_t)(void *context);

/* ---- object model ----
 * v1 simplification: dispatch_object_t is a plain void * here, not real
 * libdispatch's OS_OBJECT transparent union -- we have no ARC/objc bridge
 * to keep binary-compatible with, and every caller in this tree already
 * passes a concretely-typed dispatch_*_t, which decays to void * fine. */
typedef void *dispatch_object_t;

DISPATCH_EXPORT void dispatch_retain(dispatch_object_t object);
DISPATCH_EXPORT void dispatch_release(dispatch_object_t object);
DISPATCH_EXPORT void *dispatch_get_context(dispatch_object_t object);
DISPATCH_EXPORT void dispatch_set_context(dispatch_object_t object, void *context);

/* ---- dispatch_queue_t ---- */
typedef struct dispatch_queue_s *dispatch_queue_t;

#define DISPATCH_QUEUE_SERIAL ((void *)0)
#define DISPATCH_QUEUE_CONCURRENT ((void *)1)
typedef void *dispatch_queue_attr_t;

/* dispatch_get_global_queue's `identifier` is accepted for ABI
 * compatibility but doesn't select a differently-scheduled pool -- every
 * global queue shares this OS's one worker pool (see the QoS note in
 * docs/architecture.md). */
#define DISPATCH_QUEUE_PRIORITY_HIGH 2
#define DISPATCH_QUEUE_PRIORITY_DEFAULT 0
#define DISPATCH_QUEUE_PRIORITY_LOW (-2)
#define DISPATCH_QUEUE_PRIORITY_BACKGROUND INT16_MIN

DISPATCH_EXPORT dispatch_queue_t dispatch_queue_create(const char *label, dispatch_queue_attr_t attr);
DISPATCH_EXPORT dispatch_queue_t dispatch_get_main_queue(void);
DISPATCH_EXPORT dispatch_queue_t dispatch_get_global_queue(long identifier, unsigned long flags);
DISPATCH_EXPORT const char *dispatch_queue_get_label(dispatch_queue_t queue);
DISPATCH_EXPORT void dispatch_main(void) __attribute__((noreturn));

DISPATCH_EXPORT void dispatch_async(dispatch_queue_t queue, dispatch_block_t block);
DISPATCH_EXPORT void dispatch_sync(dispatch_queue_t queue, dispatch_block_t block);
DISPATCH_EXPORT void dispatch_async_f(dispatch_queue_t queue, void *context, dispatch_function_t work);
DISPATCH_EXPORT void dispatch_sync_f(dispatch_queue_t queue, void *context, dispatch_function_t work);

/* ---- dispatch_once ---- */
typedef long dispatch_once_t;

DISPATCH_EXPORT void dispatch_once(dispatch_once_t *predicate, dispatch_block_t block);
DISPATCH_EXPORT void dispatch_once_f(dispatch_once_t *predicate, void *context, dispatch_function_t function);

/* ---- dispatch_time_t ----
 * No kevent/kqueue timer source in this tree (nothing wires those
 * syscalls up anywhere yet) -- dispatch_after is backed by one dedicated
 * timer thread polling a sorted-deadline list, see dispatch_time.c. */
typedef uint64_t dispatch_time_t;

#define DISPATCH_TIME_NOW 0ull
#define DISPATCH_TIME_FOREVER (~0ull)

/* ---- dispatch_semaphore_t ----
 * Built on real pthread_mutex_t/pthread_cond_t, not mach semaphore_create/
 * semaphore_wait -- those are Mach traps with no syscall wrapper in this
 * tree (mach/semaphore.h is declaration-only here). */
typedef struct dispatch_semaphore_s *dispatch_semaphore_t;

DISPATCH_EXPORT dispatch_semaphore_t dispatch_semaphore_create(long value);
DISPATCH_EXPORT long dispatch_semaphore_wait(dispatch_semaphore_t sem, dispatch_time_t timeout);
DISPATCH_EXPORT long dispatch_semaphore_signal(dispatch_semaphore_t sem);

/* ---- dispatch_group_t ---- */
typedef struct dispatch_group_s *dispatch_group_t;

DISPATCH_EXPORT dispatch_group_t dispatch_group_create(void);
DISPATCH_EXPORT void dispatch_group_async(dispatch_group_t group, dispatch_queue_t queue, dispatch_block_t block);
DISPATCH_EXPORT void dispatch_group_async_f(dispatch_group_t group, dispatch_queue_t queue, void *context, dispatch_function_t work);
DISPATCH_EXPORT void dispatch_group_enter(dispatch_group_t group);
DISPATCH_EXPORT void dispatch_group_leave(dispatch_group_t group);
DISPATCH_EXPORT long dispatch_group_wait(dispatch_group_t group, dispatch_time_t timeout);
DISPATCH_EXPORT void dispatch_group_notify(dispatch_group_t group, dispatch_queue_t queue, dispatch_block_t block);

/* dispatch_after is backed by one dedicated timer thread polling a
 * sorted-deadline list (no kevent/kqueue timer source anywhere in this
 * tree) -- see dispatch_time.c. */
DISPATCH_EXPORT dispatch_time_t dispatch_time(dispatch_time_t when, int64_t delta);
DISPATCH_EXPORT dispatch_time_t dispatch_walltime(const struct timespec *when, int64_t delta);
DISPATCH_EXPORT void dispatch_after(dispatch_time_t when, dispatch_queue_t queue, dispatch_block_t block);
DISPATCH_EXPORT void dispatch_after_f(dispatch_time_t when, dispatch_queue_t queue, void *context, dispatch_function_t work);

#ifdef __cplusplus
}
#endif

#endif /* __DISPATCH_DISPATCH_H__ */
