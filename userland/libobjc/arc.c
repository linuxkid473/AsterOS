/* Copyright (c) 2026 Vihaan Nathan
 *
 * ARC runtime entry points (what clang's -fobjc-arc codegen actually
 * calls) plus the side-table refcount/weak-reference machinery backing
 * them. objc_retain/objc_release/objc_autorelease are implemented as
 * real message sends to "retain"/"release"/"autorelease" rather than
 * calling the _objc_root* helpers directly -- slower, but it means a
 * subclass override of -retain/-release/-dealloc actually runs, which a
 * direct C call would silently skip. Root.m's own -retain/-release/
 * -dealloc are what call into the _objc_root* helpers below.
 *
 * objc_retainAutoreleasedReturnValue DOES implement a (simplified)
 * fast-path reclaim -- turned out not to be optional (see its own
 * comment and autorelease.c's objc_autorelease_try_reclaim_last):
 * without it, an autoreleased value immediately claimed by the caller
 * is a genuine double-free, not just a missed optimization.
 *
 * Documented simplifications vs Apple's real ARC runtime that remain:
 * no isa-embedded inline refcount (side table only, see below), no
 * per-thread autorelease pools (see autorelease.c) -- neither is
 * something compiled .m code can observe, only performance/threading
 * characteristics this project explicitly deprioritizes (see TODO.md
 * Phase 13).
 */
#include "objc_priv.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_REFCOUNTED 4096

struct refcount_entry {
	id obj;
	uintptr_t extra; /* retain count beyond the implicit +1 from alloc */
};
static struct refcount_entry g_refcounts[MAX_REFCOUNTED];
static int g_nrefcounts;

static struct refcount_entry *
find_refcount(id obj)
{
	for (int i = 0; i < g_nrefcounts; i++) {
		if (g_refcounts[i].obj == obj) {
			return &g_refcounts[i];
		}
	}
	return 0;
}

id
_objc_rootRetain(id obj)
{
	if (!obj) {
		return obj;
	}
	struct refcount_entry *e = find_refcount(obj);
	if (e) {
		e->extra++;
	} else if (g_nrefcounts < MAX_REFCOUNTED) {
		g_refcounts[g_nrefcounts++] = (struct refcount_entry){ .obj = obj, .extra = 1 };
	}
	return obj;
}

uintptr_t
_objc_rootRetainCount(id obj)
{
	if (!obj) {
		return 0;
	}
	struct refcount_entry *e = find_refcount(obj);
	return 1 + (e ? e->extra : 0);
}

void
_objc_rootRelease(id obj)
{
	if (!obj) {
		return;
	}
	struct refcount_entry *e = find_refcount(obj);
	if (e && e->extra > 0) {
		e->extra--;
		return;
	}
	if (e) {
		/* swap-remove: this object's tracked-refcount lifetime is over,
		 * whether or not it ever actually got retained beyond +1. */
		*e = g_refcounts[--g_nrefcounts];
	}
	objc_msgSend(obj, sel_registerName("dealloc"));
}

void
_objc_rootDealloc(id obj)
{
	objc_weak_clear_object(obj);
	free(obj);
}

id
_objc_rootAlloc(Class cls)
{
	return class_createInstance(cls, 0);
}

id
_objc_rootInit(id obj)
{
	return obj;
}

Class
_objc_rootClass(id obj)
{
	return obj ? obj->isa : Nil;
}

BOOL
_objc_rootIsKindOfClass(id obj, Class cls)
{
	if (!obj) {
		return NO;
	}
	for (Class c = obj->isa; c; c = class_getSuperclass(c)) {
		if (c == cls) {
			return YES;
		}
	}
	return NO;
}

BOOL
_objc_rootRespondsToSelector(id obj, SEL sel)
{
	return obj ? class_respondsToSelector(obj->isa, sel) : NO;
}

/* ---- ARC compiler entry points ---- */

id
objc_retain(id obj)
{
	if (!obj) {
		return obj;
	}
	return objc_msgSend(obj, sel_registerName("retain"));
}

void
objc_release(id obj)
{
	if (!obj) {
		return;
	}
	objc_msgSend(obj, sel_registerName("release"));
}

id
objc_autorelease(id obj)
{
	if (!obj) {
		return obj;
	}
	return objc_msgSend(obj, sel_registerName("autorelease"));
}

id
objc_retainAutoreleasedReturnValue(id obj)
{
	if (!obj) {
		return obj;
	}
	/* If this is exactly the object the immediately preceding call
	 * autoreleased, cancel that pending pool release -- otherwise the
	 * pool's own eventual drain double-frees (see
	 * autorelease.c's objc_autorelease_try_reclaim_last). Canceling the
	 * pool entry is NOT the same as skipping the retain, though:
	 * compiled ARC code around this call site still expects to own an
	 * independent reference it will release itself later (it can't know
	 * at compile time whether the fast path was taken), so the side
	 * table still needs to record this as a real extra unit of
	 * ownership -- ground-truthed the hard way (a Counter test's dealloc
	 * fired one release too early otherwise, see TODO.md Phase 13).
	 * Real Darwin's version needs no such bookkeeping since it does
	 * neither operation for real; our side-table refcount model has to
	 * make the "one fewer future release owed" fact visible somehow. */
	if (objc_autorelease_try_reclaim_last(obj)) {
		return _objc_rootRetain(obj);
	}
	return objc_retain(obj);
}

id
objc_autoreleaseReturnValue(id obj)
{
	return objc_autorelease(obj);
}

id
objc_unsafeClaimAutoreleasedReturnValue(id obj)
{
	return obj; /* caller takes ownership without an extra retain */
}

id
objc_storeStrong(id *location, id value)
{
	id old = location ? *location : nil;
	if (value == old) {
		return value;
	}
	if (value) {
		objc_retain(value);
	}
	if (location) {
		*location = value;
	}
	if (old) {
		objc_release(old);
	}
	return value;
}

/* ---- weak references ---- */

#define MAX_WEAK_OWNERS 1024

struct weak_owner {
	id obj;
	id **slots;
	int count, cap;
};
static struct weak_owner g_weak_owners[MAX_WEAK_OWNERS];
static int g_nweak_owners;

static struct weak_owner *
find_weak_owner(id obj, int create)
{
	for (int i = 0; i < g_nweak_owners; i++) {
		if (g_weak_owners[i].obj == obj) {
			return &g_weak_owners[i];
		}
	}
	if (!create || g_nweak_owners >= MAX_WEAK_OWNERS) {
		return 0;
	}
	g_weak_owners[g_nweak_owners++] = (struct weak_owner){ .obj = obj };
	return &g_weak_owners[g_nweak_owners - 1];
}

static void
weak_owner_add_slot(id obj, id *slot)
{
	struct weak_owner *w = find_weak_owner(obj, 1);
	if (!w) {
		return;
	}
	if (w->count >= w->cap) {
		w->cap = w->cap ? w->cap * 2 : 4;
		w->slots = realloc(w->slots, sizeof(id *) * w->cap);
	}
	w->slots[w->count++] = slot;
}

static void
weak_owner_remove_slot(id obj, id *slot)
{
	struct weak_owner *w = find_weak_owner(obj, 0);
	if (!w) {
		return;
	}
	for (int i = 0; i < w->count; i++) {
		if (w->slots[i] == slot) {
			w->slots[i] = w->slots[--w->count];
			return;
		}
	}
}

void
objc_weak_clear_object(id obj)
{
	struct weak_owner *w = find_weak_owner(obj, 0);
	if (!w) {
		return;
	}
	for (int i = 0; i < w->count; i++) {
		*w->slots[i] = nil;
	}
	free(w->slots);
	*w = g_weak_owners[--g_nweak_owners];
}

id
objc_storeWeak(id *location, id value)
{
	if (location && *location) {
		weak_owner_remove_slot(*location, location);
	}
	if (value) {
		weak_owner_add_slot(value, location);
	}
	if (location) {
		*location = value;
	}
	return value;
}

id
objc_initWeak(id *location, id value)
{
	if (location) {
		*location = nil;
	}
	return objc_storeWeak(location, value);
}

void
objc_destroyWeak(id *location)
{
	objc_storeWeak(location, nil);
}

id
objc_loadWeak(id *location)
{
	return location ? *location : nil;
}

id
objc_copyWeak(id *dest, id *src)
{
	return objc_initWeak(dest, src ? *src : nil);
}

void
objc_moveWeak(id *dest, id *src)
{
	objc_initWeak(dest, src ? *src : nil);
	objc_destroyWeak(src);
}
