/* Copyright (c) 2026 Vihaan Nathan
 *
 * dispatch_queue_t, the global worker pool, and dispatch_async/dispatch_sync.
 *
 * Scheduling invariant: a serial queue never has two items draining at
 * once. Enforced without a dedicated thread per serial queue by having
 * whichever worker starts draining a serial queue hold `draining` for the
 * queue's *entire* backlog, not just one item -- while `draining` is set,
 * push_item() sees it and skips scheduling a runnable-list node (the
 * drainer will see the new item itself on its next lock/pop, since both
 * push and pop take q->lock). Concurrent queues skip the `draining` gate
 * entirely: one runnable-list node per pushed item, any worker may pop and
 * run one independently.
 *
 * dispatch_get_main_queue() is v1-simplified to an ordinary auto-draining
 * serial queue drained by the pool like any other -- not the real
 * runloop-attached main queue (there's no CFRunLoop/dispatch-source
 * integration to hook it to yet). dispatch_main() therefore doesn't drain
 * anything itself; it just parks the calling thread forever, matching the
 * real ABI contract (never returns) while being honest that blocks
 * submitted to the main queue actually run on a pool worker, not the
 * thread that called dispatch_main().
 */
#include "dispatch_internal.h"
#include <Block.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/sysctl.h>
#include <time.h>

pthread_key_t g_dispatch_draining_key;

static dispatch_queue_t g_main_queue;
static dispatch_queue_t g_global_queue;

static struct {
	pthread_mutex_t lock;
	pthread_cond_t cond;
	struct dispatch_runnable_node *head, *tail;
} g_runnable = { PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, NULL, NULL };

static void
runnable_push(dispatch_queue_t q)
{
	struct dispatch_runnable_node *node = malloc(sizeof(*node));
	node->queue = q;
	node->next = NULL;
	pthread_mutex_lock(&g_runnable.lock);
	if (g_runnable.tail) {
		g_runnable.tail->next = node;
	} else {
		g_runnable.head = node;
	}
	g_runnable.tail = node;
	pthread_cond_signal(&g_runnable.cond);
	pthread_mutex_unlock(&g_runnable.lock);
}

static struct dispatch_runnable_node *
runnable_pop_blocking(void)
{
	pthread_mutex_lock(&g_runnable.lock);
	while (!g_runnable.head) {
		pthread_cond_wait(&g_runnable.cond, &g_runnable.lock);
	}
	struct dispatch_runnable_node *node = g_runnable.head;
	g_runnable.head = node->next;
	if (!g_runnable.head) {
		g_runnable.tail = NULL;
	}
	pthread_mutex_unlock(&g_runnable.lock);
	return node;
}

/* must be called with q->lock held */
static struct dispatch_item *
queue_pop_head_locked(dispatch_queue_t q)
{
	struct dispatch_item *item = q->head;
	if (item) {
		q->head = item->next;
		if (!q->head) {
			q->tail = NULL;
		}
	}
	return item;
}

void
_dispatch_queue_push_item(dispatch_queue_t q, dispatch_function_t func, void *context)
{
	struct dispatch_item *item = malloc(sizeof(*item));
	item->func = func;
	item->context = context;
	item->next = NULL;

	bool should_push = false;
	pthread_mutex_lock(&q->lock);
	if (q->tail) {
		q->tail->next = item;
	} else {
		q->head = item;
	}
	q->tail = item;
	if (q->is_serial) {
		if (!q->draining && !q->on_runnable_list) {
			q->on_runnable_list = true;
			should_push = true;
		}
	} else {
		should_push = true;
	}
	pthread_mutex_unlock(&q->lock);

	if (should_push) {
		runnable_push(q);
	}
}

static void
_dispatch_queue_destroy(void *self)
{
	dispatch_queue_t q = self;
	free((void *)q->label);
	pthread_mutex_destroy(&q->lock);
	free(q);
}

dispatch_queue_t
dispatch_queue_create(const char *label, dispatch_queue_attr_t attr)
{
	dispatch_queue_t q = malloc(sizeof(*q));
	_dispatch_object_init(&q->hdr, _dispatch_queue_destroy);
	q->label = label ? strdup(label) : NULL;
	q->is_serial = (attr != DISPATCH_QUEUE_CONCURRENT);
	pthread_mutex_init(&q->lock, NULL);
	q->head = q->tail = NULL;
	q->draining = false;
	q->on_runnable_list = false;
	return q;
}

dispatch_queue_t
dispatch_get_main_queue(void)
{
	return g_main_queue;
}

dispatch_queue_t
dispatch_get_global_queue(long identifier, unsigned long flags)
{
	(void)identifier; (void)flags;
	return g_global_queue;
}

const char *
dispatch_queue_get_label(dispatch_queue_t queue)
{
	return queue->label ? queue->label : "";
}

void
dispatch_main(void)
{
	for (;;) {
		struct timespec ts = { 3600, 0 };
		nanosleep(&ts, NULL);
	}
}

static void
_dispatch_call_block_invoke(void *context)
{
	dispatch_block_t b = context;
	b();
	Block_release(b);
}

/* dispatch_sync's trampoline runs on whichever worker drains the target
 * queue; the block/function pointer itself doesn't need Block_copy here
 * (unlike dispatch_async) since dispatch_sync_f blocks the calling stack
 * frame until this signals -- it's guaranteed still alive. */
static void
_dispatch_call_stack_block_invoke(void *context)
{
	dispatch_block_t b = context;
	b();
}

struct sync_ctx {
	void *context;
	dispatch_function_t work;
	dispatch_semaphore_t done;
};

static void
_dispatch_sync_trampoline(void *context)
{
	struct sync_ctx *sc = context;
	sc->work(sc->context);
	dispatch_semaphore_signal(sc->done);
}

void
dispatch_async_f(dispatch_queue_t queue, void *context, dispatch_function_t work)
{
	_dispatch_queue_push_item(queue, work, context);
}

void
dispatch_async(dispatch_queue_t queue, dispatch_block_t block)
{
	dispatch_async_f(queue, Block_copy(block), _dispatch_call_block_invoke);
}

void
dispatch_sync_f(dispatch_queue_t queue, void *context, dispatch_function_t work)
{
	if (pthread_getspecific(g_dispatch_draining_key) == queue) {
		fprintf(stderr, "dispatch_sync: deadlock: targeting a queue this thread is already draining\n");
		abort();
	}
	struct sync_ctx sc = { context, work, dispatch_semaphore_create(0) };
	_dispatch_queue_push_item(queue, _dispatch_sync_trampoline, &sc);
	dispatch_semaphore_wait(sc.done, DISPATCH_TIME_FOREVER);
	dispatch_release(sc.done);
}

void
dispatch_sync(dispatch_queue_t queue, dispatch_block_t block)
{
	dispatch_sync_f(queue, block, _dispatch_call_stack_block_invoke);
}

static int
get_ncpu(void)
{
	int ncpu = 1;
	size_t len = sizeof(ncpu);
	int mib[2] = { CTL_HW, HW_NCPU };
	if (sysctl(mib, 2, &ncpu, &len, NULL, 0) != 0 || ncpu < 1) {
		ncpu = 1;
	}
	return ncpu;
}

static void *
worker_main(void *arg)
{
	(void)arg;
	for (;;) {
		struct dispatch_runnable_node *node = runnable_pop_blocking();
		dispatch_queue_t q = node->queue;
		free(node);

		if (q->is_serial) {
			pthread_mutex_lock(&q->lock);
			q->on_runnable_list = false;
			q->draining = true;
			for (;;) {
				struct dispatch_item *item = queue_pop_head_locked(q);
				if (!item) {
					q->draining = false;
					pthread_mutex_unlock(&q->lock);
					break;
				}
				pthread_mutex_unlock(&q->lock);
				pthread_setspecific(g_dispatch_draining_key, q);
				item->func(item->context);
				pthread_setspecific(g_dispatch_draining_key, NULL);
				free(item);
				pthread_mutex_lock(&q->lock);
			}
		} else {
			pthread_mutex_lock(&q->lock);
			struct dispatch_item *item = queue_pop_head_locked(q);
			pthread_mutex_unlock(&q->lock);
			if (item) {
				item->func(item->context);
				free(item);
			}
		}
	}
	return NULL;
}

void
_dispatch_workerpool_start(void)
{
	g_main_queue = dispatch_queue_create("com.apple.main-thread", DISPATCH_QUEUE_SERIAL);
	g_global_queue = dispatch_queue_create("com.apple.root.default-qos", DISPATCH_QUEUE_CONCURRENT);
	pthread_key_create(&g_dispatch_draining_key, NULL);

	int n = get_ncpu();
	if (n < 2) {
		n = 2;
	}
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	for (int i = 0; i < n; i++) {
		pthread_t t;
		pthread_create(&t, &attr, worker_main, NULL);
	}
	pthread_attr_destroy(&attr);
}
