/* Copyright (c) 2026 Vihaan Nathan
 *
 * Runtime-internal bookkeeping: everything here is *our own* layout,
 * never read by clang-emitted code or anything outside this runtime, so
 * none of it is ABI-constrained the way objc_abi.h is. Fixed-capacity
 * arrays with linear search throughout (selectors/classes/protocols) --
 * adequate for the handful of classes a test program or minimal launchd
 * subsystem defines; a real hash table is future work if this runtime
 * ever needs to carry a Foundation-scale app (see TODO.md Phase 13).
 */
#ifndef OBJC_PRIV_H
#define OBJC_PRIV_H

#include "objc_abi.h"
#include <objc/runtime.h>
#include <objc/message.h>
#include <stddef.h>
#include <mach-o/loader.h>

#define OBJC_MAX_SELECTORS 4096
#define OBJC_MAX_CLASSES 512
#define OBJC_MAX_PROTOCOLS 128
#define OBJC_CACHE_SIZE 16

/* Live, mutable per-class state -- what class_t.data points to once a
 * class is realized (before that, it points at the compiler's
 * class_ro_t; see objc_abi.h's comment on class_t.data). Method/
 * protocol/property lists here are flattened copies (base + every
 * attached category), since categories can attach after the class's own
 * class_ro_t was emitted and that struct is never mutated in place. */
struct class_rw_t {
	struct class_ro_t *ro;

	struct method_t *methods;
	int method_count, method_cap;

	struct protocol_t **protocols;
	int protocol_count, protocol_cap;

	struct property_t *properties;
	int property_count, property_cap;

	/* Only used for objc_allocateClassPair-created classes (class_addIvar
	 * before objc_registerClassPair) -- compiled classes' ivars are read
	 * straight from ro->ivars, never copied here. */
	struct ivar_t *extra_ivars;
	int extra_ivar_count, extra_ivar_cap;

	uint32_t instance_size;
	int realized;
};

struct cache_bucket {
	SEL sel;
	IMP imp;
};

struct method_cache {
	struct cache_bucket buckets[OBJC_CACHE_SIZE];
};

static inline struct class_t *
as_class_t(Class cls)
{
	return (struct class_t *)(void *)cls;
}

/* class_t.data is either a compiler-emitted class_ro_t* (not yet
 * realized) or our own class_rw_t* (realized) -- neither is ever NULL,
 * so the two can't be told apart by nullness alone. Both are real
 * pointers to (at least) 8-byte-aligned structs, so bit 0 is always 0
 * in a genuine, untagged class_ro_t*; we use it as the "realized" flag,
 * set only when we replace data with our own class_rw_t. Caught the
 * hard way: without this, realize_class()'s "already realized?" guard
 * (originally just `class_rw(cls) != NULL`) was true on every class's
 * very first call, since a plain cast of the still-class_ro_t pointer
 * is never NULL either -- nothing ever got realized or registered. */
#define CLASS_RW_TAG 1

static inline struct class_rw_t *
class_rw(Class cls)
{
	uintptr_t data = (uintptr_t)as_class_t(cls)->data;
	if (!(data & CLASS_RW_TAG)) {
		return 0;
	}
	return (struct class_rw_t *)(data & ~(uintptr_t)CLASS_RW_TAG);
}

static inline void
class_set_rw(Class cls, struct class_rw_t *rw)
{
	as_class_t(cls)->data = (struct class_ro_t *)((uintptr_t)rw | CLASS_RW_TAG);
}

/* selector.c */
void objc_selector_init(void);
void objc_selref_fixup(SEL *ref);

/* class.c */
void objc_register_image(const struct mach_header_64 *mh);
void objc_attach_categories(void);
void objc_register_dynamic_class(Class cls);
Class class_lookup_or_panic(const char *name);
struct method_t *class_lookup_method_t(Class cls, SEL sel);
void rw_append_method(struct class_rw_t *rw, SEL name, const char *types, IMP imp);
void rw_append_protocol(struct class_rw_t *rw, struct protocol_t *p);

/* msgSend.S / dispatch.c */
IMP objc_class_lookup_and_cache(Class cls, SEL sel);

/* runtime internals shared with arc.c/autorelease.c/Root.m */
id _objc_rootAlloc(Class cls);
id _objc_rootInit(id obj);
void _objc_rootDealloc(id obj);
id _objc_rootRetain(id obj);
void _objc_rootRelease(id obj);
id _objc_rootAutorelease(id obj);
uintptr_t _objc_rootRetainCount(id obj);
Class _objc_rootClass(id obj);
BOOL _objc_rootIsKindOfClass(id obj, Class cls);
BOOL _objc_rootRespondsToSelector(id obj, SEL sel);

void objc_weak_clear_object(id obj);

/* arc.c -- ARC compiler entry points, also used internally (e.g.
 * autorelease.c draining a pool). Real .m callers get these for free
 * from -fobjc-arc codegen without needing this header at all; declared
 * here purely so this runtime's own C files can call them too. */
id objc_retain(id obj);
void objc_release(id obj);
id objc_autorelease(id obj);
id objc_storeStrong(id *location, id value);
void *objc_autoreleasePoolPush(void);
void objc_autoreleasePoolPop(void *token);
int objc_autorelease_try_reclaim_last(id obj);

#endif /* OBJC_PRIV_H */
