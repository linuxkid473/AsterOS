/* Copyright (c) 2026 Vihaan Nathan
 *
 * Selector uniquing. SEL is a uniqued const char* under the hood (the
 * traditional convention, correct for this ABI generation -- the
 * relative/indexed selector scheme is a newer arm64e-only optimization).
 * __DATA,__objc_selrefs slots start out pointing at each image's own
 * __objc_methname string; objc_selref_fixup rewrites them in place to
 * point at the single uniqued instance, exactly like real dyld/objc.
 */
#include "objc_priv.h"
#include <string.h>
#include <stdlib.h>

static const char *g_selectors[OBJC_MAX_SELECTORS];
static int g_nselectors;

void
objc_selector_init(void)
{
	/* nothing to do -- table starts empty; kept as a named entry point
	 * for symmetry with class.c's init-shaped functions and in case
	 * future work needs to seed well-known selectors up front. */
}

SEL
sel_registerName(const char *str)
{
	if (!str) {
		return (SEL)0;
	}
	for (int i = 0; i < g_nselectors; i++) {
		if (strcmp(g_selectors[i], str) == 0) {
			return (SEL)(void *)g_selectors[i];
		}
	}
	if (g_nselectors >= OBJC_MAX_SELECTORS) {
		return (SEL)0; /* out of table space -- caller sees an unusable SEL */
	}
	size_t len = strlen(str) + 1;
	char *copy = malloc(len);
	if (!copy) {
		return (SEL)0;
	}
	memcpy(copy, str, len);
	g_selectors[g_nselectors++] = copy;
	return (SEL)(void *)copy;
}

const char *
sel_getName(SEL sel)
{
	return sel ? (const char *)(void *)sel : "<null selector>";
}

BOOL
sel_isEqual(SEL lhs, SEL rhs)
{
	return lhs == rhs;
}

void
objc_selref_fixup(SEL *ref)
{
	const char *raw = (const char *)(void *)*ref;
	*ref = sel_registerName(raw);
}
