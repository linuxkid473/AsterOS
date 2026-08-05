/* Copyright (c) 2026 Vihaan Nathan
 *
 * dispatch_time_t is absolute nanoseconds since the Unix epoch
 * (CLOCK_REALTIME) -- deliberately the same epoch pthread_cond_timedwait
 * uses internally (gettimeofday-based, see pthread.c), so a dispatch_time_t
 * can be split into a struct timespec and handed straight to
 * pthread_cond_timedwait without another conversion (dispatch_semaphore.c/
 * dispatch_group.c do exactly that).
 *
 * dispatch_after is backed by one dedicated timer thread polling a
 * sorted-deadline list -- there is no kevent/kqueue timer source wired up
 * anywhere in this tree, so a real EVFILT_TIMER dispatch_source isn't an
 * option this phase. nanosleep()-capped poll rather than a cond_wait:
 * pthread_cond_timedwait here is itself a spin-poll against gettimeofday
 * (see pthread.c), so a plain capped nanosleep is no less efficient and
 * needs no extra synchronization for "a nearer deadline just got added".
 */
#include "dispatch_internal.h"
#include <Block.h>
#include <stdlib.h>
#include <time.h>

#define TIMER_POLL_CAP_NS 20000000ull /* 20ms -- worst-case dispatch_after() lateness */

static inline void
spin_pause(void)
{
	__asm__ __volatile__("pause" ::: "memory");
}

struct timer_node {
	struct timer_node *next;
	dispatch_time_t deadline;
	dispatch_queue_t queue;
	dispatch_function_t func;
	void *context;
};

static struct {
	pthread_mutex_t lock;
	struct timer_node *head; /* sorted ascending by deadline */
} g_timers = { PTHREAD_MUTEX_INITIALIZER, NULL };

static uint64_t
now_ns(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
	return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static dispatch_time_t
add_delta(uint64_t base, int64_t delta)
{
	if (delta >= 0) {
		return base + (uint64_t)delta;
	}
	uint64_t d = (uint64_t)(-delta);
	return base > d ? base - d : 0;
}

dispatch_time_t
dispatch_time(dispatch_time_t when, int64_t delta)
{
	uint64_t base = (when == DISPATCH_TIME_NOW) ? now_ns() : when;
	return add_delta(base, delta);
}

dispatch_time_t
dispatch_walltime(const struct timespec *when, int64_t delta)
{
	uint64_t base = when ? (uint64_t)when->tv_sec * 1000000000ull + (uint64_t)when->tv_nsec : now_ns();
	return add_delta(base, delta);
}

void
_dispatch_timer_schedule(dispatch_time_t when, dispatch_queue_t queue, dispatch_function_t func, void *context)
{
	struct timer_node *node = malloc(sizeof(*node));
	node->deadline = when;
	node->queue = queue;
	node->func = func;
	node->context = context;

	pthread_mutex_lock(&g_timers.lock);
	struct timer_node **pp = &g_timers.head;
	while (*pp && (*pp)->deadline <= when) {
		pp = &(*pp)->next;
	}
	node->next = *pp;
	*pp = node;
	pthread_mutex_unlock(&g_timers.lock);
}

void
dispatch_after_f(dispatch_time_t when, dispatch_queue_t queue, void *context, dispatch_function_t work)
{
	if (when <= now_ns()) {
		_dispatch_queue_push_item(queue, work, context);
		return;
	}
	_dispatch_timer_schedule(when, queue, work, context);
}

static void
_dispatch_after_block_invoke(void *context)
{
	dispatch_block_t b = context;
	b();
	Block_release(b);
}

void
dispatch_after(dispatch_time_t when, dispatch_queue_t queue, dispatch_block_t block)
{
	dispatch_after_f(when, queue, Block_copy(block), _dispatch_after_block_invoke);
}

static void *
timer_main(void *arg)
{
	(void)arg;
	for (;;) {
		pthread_mutex_lock(&g_timers.lock);
		uint64_t now = now_ns();
		while (g_timers.head && g_timers.head->deadline <= now) {
			struct timer_node *fire = g_timers.head;
			g_timers.head = fire->next;
			pthread_mutex_unlock(&g_timers.lock);
			_dispatch_queue_push_item(fire->queue, fire->func, fire->context);
			free(fire);
			pthread_mutex_lock(&g_timers.lock);
			now = now_ns();
		}
		uint64_t sleep_ns = TIMER_POLL_CAP_NS;
		if (g_timers.head) {
			uint64_t remaining = g_timers.head->deadline > now ? g_timers.head->deadline - now : 0;
			if (remaining < sleep_ns) {
				sleep_ns = remaining;
			}
		}
		pthread_mutex_unlock(&g_timers.lock);
		/* Deliberately not nanosleep() here: it's built on a process-wide
		 * ITIMER_REAL + SIGALRM + sigsuspend() (time.c), and real xnu's
		 * realitexpire() (kern_time.c) delivers that SIGALRM via
		 * psignal() -- a process-directed signal with no guarantee it
		 * lands on *this* thread's sigsuspend() rather than one of the
		 * worker pool's. Ground-truthed live: with the worker pool
		 * threads also running, the timer thread's nanosleep()
		 * intermittently wedged forever (a different thread's sigsuspend
		 * silently consumed the signal instead), so dispatch_after never
		 * fired. sched_yield() is a documented no-op in this tree (no
		 * cheap kernel yield primitive wired up), so a spin-wait relying
		 * on xnu's own preemptive quantum expiry -- same tradeoff
		 * pthread_mutex_t/pthread_cond_timedwait already make -- is the
		 * only safe option here. */
		if (sleep_ns > 0) {
			uint64_t wake_at = now_ns() + sleep_ns;
			while (now_ns() < wake_at) {
				spin_pause();
			}
		}
	}
	return NULL;
}

void
_dispatch_timer_thread_start(void)
{
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_t t;
	pthread_create(&t, &attr, timer_main, NULL);
	pthread_attr_destroy(&attr);
}
