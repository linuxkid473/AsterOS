/* Copyright (c) 2026 Vihaan Nathan
 *
 * dispatch_once: atomic-CAS spin, same idiom as pthread.c's own internal
 * spinlock (0 = never run, 1 = running, 2 = done).
 */
#include "dispatch_internal.h"
#include <Block.h>

#define DISPATCH_ONCE_UNSTARTED 0
#define DISPATCH_ONCE_RUNNING 1
#define DISPATCH_ONCE_DONE 2

static inline void
spin_pause(void)
{
	__asm__ __volatile__("pause" ::: "memory");
}

void
dispatch_once_f(dispatch_once_t *predicate, void *context, dispatch_function_t function)
{
	long expected = DISPATCH_ONCE_UNSTARTED;
	if (__atomic_compare_exchange_n(predicate, &expected, DISPATCH_ONCE_RUNNING, false,
	    __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) {
		function(context);
		__atomic_store_n(predicate, DISPATCH_ONCE_DONE, __ATOMIC_RELEASE);
		return;
	}
	while (__atomic_load_n(predicate, __ATOMIC_ACQUIRE) != DISPATCH_ONCE_DONE) {
		spin_pause();
	}
}

static void
_dispatch_once_block_invoke(void *context)
{
	dispatch_block_t b = context;
	b();
}

void
dispatch_once(dispatch_once_t *predicate, dispatch_block_t block)
{
	/* No Block_copy needed -- dispatch_once_f runs `block` synchronously
	 * on the calling thread, same reasoning as dispatch_sync's stack
	 * trampoline. */
	dispatch_once_f(predicate, block, _dispatch_once_block_invoke);
}
