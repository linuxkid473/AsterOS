/* Copyright (c) 2026 Vihaan Nathan
 *
 * Private structs every dispatch object and the worker pool are built on.
 * Not part of the public API, same spirit as CoreFoundation/CFInternal.h.
 *
 * Refcounting: every concrete object embeds struct dispatch_object_hdr as
 * its first member (same "base struct is the first field" idiom
 * CFRuntimeBase/objc_object use), touched only via __atomic_* builtins --
 * see pthread.c's own spinlock convention.
 */
#ifndef DISPATCH_INTERNAL_H
#define DISPATCH_INTERNAL_H

#include <dispatch/dispatch.h>
#include <pthread.h>
#include <stdbool.h>

struct dispatch_object_hdr {
	volatile int refcount;
	void *context;			/* dispatch_get_context/dispatch_set_context */
	void (*destroy)(void *self);	/* called once refcount hits 0 */
};

void _dispatch_object_init(struct dispatch_object_hdr *hdr, void (*destroy)(void *self));

/* One work item: a copied block or an `_f` function pointer + context,
 * queued on a dispatch_queue_t's FIFO. Every enqueue path Block_copy's the
 * block before this is created -- the caller's stack frame may be long
 * gone by the time a worker thread drains it. */
struct dispatch_item {
	struct dispatch_item *next;
	dispatch_function_t func;
	void *context;
};

/* Global runnable-queue list every worker pthread blocks on -- pushed to
 * whenever a queue transitions from "nothing to drain" to "has work and
 * isn't already being drained" (see dispatch_queue.c's draining-flag
 * protocol). Concurrent queues can appear on this list more than once
 * concurrently; serial queues never do. */
struct dispatch_runnable_node {
	struct dispatch_runnable_node *next;
	dispatch_queue_t queue;
};

struct dispatch_queue_s {
	struct dispatch_object_hdr hdr;
	const char *label;
	bool is_serial;
	pthread_mutex_t lock;
	struct dispatch_item *head, *tail;
	bool draining;		/* serial queues only -- see dispatch_queue.c */
	bool on_runnable_list;	/* coalesces duplicate pushes for one queue */
};

struct dispatch_semaphore_s {
	struct dispatch_object_hdr hdr;
	pthread_mutex_t lock;
	pthread_cond_t cond;
	long value;
};

struct dispatch_group_s {
	struct dispatch_object_hdr hdr;
	pthread_mutex_t lock;
	pthread_cond_t cond;
	long count;
	/* one-shot notify list, run once `count` returns to 0 */
	struct dispatch_group_notify {
		struct dispatch_group_notify *next;
		dispatch_queue_t queue;
		dispatch_function_t func;
		void *context;
	} *notify_head;
};

/* dispatch_queue.c */
void _dispatch_workerpool_start(void);
void _dispatch_queue_push_item(dispatch_queue_t q, dispatch_function_t func, void *context);
/* thread-local "queue I'm currently draining", used by dispatch_sync's
 * same-queue deadlock check -- set/cleared by the worker pool drain loop,
 * read (not written) by application code via dispatch_sync. */
extern pthread_key_t g_dispatch_draining_key;

/* dispatch_time.c */
void _dispatch_timer_thread_start(void);
void _dispatch_timer_schedule(dispatch_time_t when, dispatch_queue_t queue, dispatch_function_t func, void *context);

/* dispatch_init.c calls both starts once per process. */

#endif /* DISPATCH_INTERNAL_H */
