/* Copyright (c) 2026 Vihaan Nathan
 *
 * The C half of message dispatch -- objc_msgSend.S saves every argument
 * register, calls objc_class_lookup_and_cache(cls, sel) to get an IMP,
 * restores the registers, and jmps to it. The cache bucket format here
 * is entirely our own (nothing outside this runtime ever reads it), so
 * it's a plain fixed-size open-addressing table per class rather than
 * anything trying to match Apple's actual cache_t layout.
 */
#include "objc_priv.h"
#include <stdio.h>
#include <stdlib.h>

/* Every class_t/metaclass_t clang emits initializes its `cache` field to
 * the address of this symbol (ground-truthed via probe .o relocations --
 * "__objc_empty_cache", referenced directly by name, real objc4 exports
 * the same symbol for the same reason). It must be a real, validly-sized
 * struct method_cache -- reading it as "all buckets empty" is correct
 * and safe, but it's shared read-only across every not-yet-realized
 * class, so a cache fill must replace the pointer with a private
 * allocation rather than writing into this sentinel. */
struct method_cache _objc_empty_cache;

static unsigned
cache_index(SEL sel)
{
	return (unsigned)(((uintptr_t)sel >> 4) & (OBJC_CACHE_SIZE - 1));
}

static struct method_cache *
ensure_cache(Class cls)
{
	struct class_t *ct = as_class_t(cls);
	if (!ct->cache || ct->cache == &_objc_empty_cache) {
		ct->cache = calloc(1, sizeof(struct method_cache));
	}
	return (struct method_cache *)ct->cache;
}

static void
objc_does_not_recognize(Class cls, SEL sel)
{
	fprintf(stderr, "libobjc: -[%s %s]: unrecognized selector\n",
	    cls ? class_getName(cls) : "(nil class)", sel_getName(sel));
	abort();
}

IMP
objc_class_lookup_and_cache(Class cls, SEL sel)
{
	struct method_cache *cache = ensure_cache(cls);
	unsigned idx = cache_index(sel);

	if (cache) {
		for (unsigned i = 0; i < OBJC_CACHE_SIZE; i++) {
			struct cache_bucket *b = &cache->buckets[(idx + i) % OBJC_CACHE_SIZE];
			if (b->sel == sel) {
				return b->imp;
			}
			if (!b->sel) {
				break; /* empty slot: not cached, fall through to a real lookup */
			}
		}
	}

	struct method_t *m = class_lookup_method_t(cls, sel);
	if (!m) {
		objc_does_not_recognize(cls, sel);
	}

	if (cache) {
		for (unsigned i = 0; i < OBJC_CACHE_SIZE; i++) {
			struct cache_bucket *b = &cache->buckets[(idx + i) % OBJC_CACHE_SIZE];
			if (!b->sel || b->sel == sel) {
				b->sel = sel;
				b->imp = m->imp;
				break;
			}
		}
	}
	return m->imp;
}
