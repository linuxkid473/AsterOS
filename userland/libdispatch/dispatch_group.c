/* Copyright (c) 2026 Vihaan Nathan
 *
 * dispatch_group_t: an atomic outstanding-count plus a one-shot notify
 * list, run once the count returns to zero.
 */
#include "dispatch_internal.h"
#include <Block.h>
#include <stdlib.h>

static void
_dispatch_group_destroy(void *self)
{
	dispatch_group_t g = self;
	pthread_mutex_destroy(&g->lock);
	pthread_cond_destroy(&g->cond);
	free(g);
}

dispatch_group_t
dispatch_group_create(void)
{
	dispatch_group_t g = malloc(sizeof(*g));
	_dispatch_object_init(&g->hdr, _dispatch_group_destroy);
	pthread_mutex_init(&g->lock, NULL);
	pthread_cond_init(&g->cond, NULL);
	g->count = 0;
	g->notify_head = NULL;
	return g;
}

void
dispatch_group_enter(dispatch_group_t g)
{
	pthread_mutex_lock(&g->lock);
	g->count++;
	pthread_mutex_unlock(&g->lock);
}

void
dispatch_group_leave(dispatch_group_t g)
{
	pthread_mutex_lock(&g->lock);
	g->count--;
	struct dispatch_group_notify *pending = NULL;
	if (g->count == 0) {
		pending = g->notify_head;
		g->notify_head = NULL;
		pthread_cond_broadcast(&g->cond);
	}
	pthread_mutex_unlock(&g->lock);

	while (pending) {
		struct dispatch_group_notify *next = pending->next;
		_dispatch_queue_push_item(pending->queue, pending->func, pending->context);
		free(pending);
		pending = next;
	}
}

struct dispatch_group_async_ctx {
	dispatch_group_t g;
	dispatch_function_t work;
	void *work_context;
};

static void
_dispatch_group_async_trampoline(void *context)
{
	struct dispatch_group_async_ctx *ctx = context;
	ctx->work(ctx->work_context);
	dispatch_group_leave(ctx->g);
	free(ctx);
}

void
dispatch_group_async_f(dispatch_group_t g, dispatch_queue_t queue, void *context, dispatch_function_t work)
{
	struct dispatch_group_async_ctx *ctx = malloc(sizeof(*ctx));
	ctx->g = g;
	ctx->work = work;
	ctx->work_context = context;
	dispatch_group_enter(g);
	_dispatch_queue_push_item(queue, _dispatch_group_async_trampoline, ctx);
}

static void
_dispatch_group_block_invoke(void *context)
{
	dispatch_block_t b = context;
	b();
	Block_release(b);
}

void
dispatch_group_async(dispatch_group_t g, dispatch_queue_t queue, dispatch_block_t block)
{
	dispatch_group_async_f(g, queue, Block_copy(block), _dispatch_group_block_invoke);
}

long
dispatch_group_wait(dispatch_group_t g, dispatch_time_t timeout)
{
	pthread_mutex_lock(&g->lock);
	long result = 0;
	if (timeout == DISPATCH_TIME_FOREVER) {
		while (g->count > 0) {
			pthread_cond_wait(&g->cond, &g->lock);
		}
	} else if (timeout == DISPATCH_TIME_NOW) {
		result = g->count > 0 ? -1 : 0;
	} else {
		struct timespec ts = { (time_t)(timeout / 1000000000ull), (long)(timeout % 1000000000ull) };
		while (g->count > 0) {
			if (pthread_cond_timedwait(&g->cond, &g->lock, &ts) != 0 && g->count > 0) {
				result = -1;
				break;
			}
		}
	}
	pthread_mutex_unlock(&g->lock);
	return result;
}

void
dispatch_group_notify(dispatch_group_t g, dispatch_queue_t queue, dispatch_block_t block)
{
	dispatch_block_t copy = Block_copy(block);
	struct dispatch_group_notify *node = malloc(sizeof(*node));
	node->queue = queue;
	node->func = _dispatch_group_block_invoke;
	node->context = copy;
	node->next = NULL;

	pthread_mutex_lock(&g->lock);
	bool fire_now = (g->count == 0);
	if (!fire_now) {
		node->next = g->notify_head;
		g->notify_head = node;
	}
	pthread_mutex_unlock(&g->lock);

	if (fire_now) {
		_dispatch_queue_push_item(node->queue, node->func, node->context);
		free(node);
	}
}
