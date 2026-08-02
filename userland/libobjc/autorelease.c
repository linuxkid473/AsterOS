/* Copyright (c) 2026 Vihaan Nathan
 *
 * Autorelease pools: a single global stack, not per-thread. This
 * project has no real threading yet (pthread_stub.c always fails
 * pthread_create), so a thread-local pool stack would be dead
 * complexity with nothing to exercise it -- documented simplification,
 * revisit if/when real threads exist.
 */
#include "objc_priv.h"
#include <stdio.h>
#include <stdlib.h>

#define MAX_AUTORELEASED 4096
#define MAX_POOL_DEPTH 256

static id g_autorelease_stack[MAX_AUTORELEASED];
static int g_autorelease_top;

static int g_pool_marks[MAX_POOL_DEPTH];
static int g_pool_depth;

id
_objc_rootAutorelease(id obj)
{
	if (!obj) {
		return obj;
	}
	if (g_autorelease_top >= MAX_AUTORELEASED) {
		fprintf(stderr, "libobjc: autorelease pool overflow (max %d pending)\n", MAX_AUTORELEASED);
		abort();
	}
	g_autorelease_stack[g_autorelease_top++] = obj;
	return obj;
}

/* Real ARC codegen calls objc_retainAutoreleasedReturnValue after nearly
 * every id-returning message send whose result is kept (init, autorelease
 * itself, etc.) -- it can't know at compile time whether the callee just
 * autoreleased that exact value, so it must be able to *cancel* a
 * just-pushed autorelease instead of unconditionally retaining. Skipping
 * this (as an earlier version of this runtime did, treating
 * retainAutoreleasedReturnValue as a plain retain always) isn't just
 * slower -- it's wrong: a real retain PLUS an uncanceled pending pool
 * release double-frees, ground-truthed the hard way with a Counter test
 * whose dealloc count came out one too high (see TODO.md Phase 13).
 * Apple's real mechanism inspects the return address / a thread-local
 * flag the callee sets; single-threaded and call-stack-inspection-free
 * here, so this approximates it with "is the top of the autorelease
 * stack literally this object" -- correct whenever the two calls are
 * adjacent, which is the only case real -fobjc-arc codegen produces. */
int
objc_autorelease_try_reclaim_last(id obj)
{
	if (g_autorelease_top > 0 && g_autorelease_stack[g_autorelease_top - 1] == obj) {
		g_autorelease_top--;
		return 1;
	}
	return 0;
}

void *
objc_autoreleasePoolPush(void)
{
	if (g_pool_depth >= MAX_POOL_DEPTH) {
		fprintf(stderr, "libobjc: autorelease pool nesting too deep (max %d)\n", MAX_POOL_DEPTH);
		abort();
	}
	g_pool_marks[g_pool_depth] = g_autorelease_top;
	return &g_pool_marks[g_pool_depth++];
}

void
objc_autoreleasePoolPop(void *token)
{
	int idx = (int)((int *)token - g_pool_marks);
	if (idx < 0 || idx >= g_pool_depth) {
		return; /* stale/unbalanced token -- nothing safe to do */
	}
	int mark = g_pool_marks[idx];
	for (int i = g_autorelease_top - 1; i >= mark; i--) {
		objc_release(g_autorelease_stack[i]);
	}
	g_autorelease_top = mark;
	g_pool_depth = idx;
}
