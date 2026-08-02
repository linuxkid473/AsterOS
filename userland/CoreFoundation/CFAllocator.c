/* Copyright (c) 2026 Vihaan Nathan
 *
 * v1: no custom allocator contexts (CFAllocatorCreate isn't implemented
 * at all) -- every named allocator is the same malloc-backed singleton.
 * See CFBase.h's own header comment for why this is a documented cut,
 * not an oversight.
 */
#include "CFInternal.h"
#include <stdlib.h>

struct __CFAllocator {
	CFRuntimeBase base;
};

static CFTypeID g_allocatorTypeID;
static struct __CFAllocator g_defaultAllocator;
static CFAllocatorRef g_currentDefault;

/* kCFAllocatorDefault et al are static-storage constants that client
 * code can dereference (CFGetTypeID/CFEqual) before ever calling any
 * other CF entry point, unlike the dynamically-created types elsewhere
 * in this tree -- so this class registers itself via a load-time
 * constructor (dyld runs __DATA,__mod_init_func for every image, see
 * macho_load.c) instead of the usual pthread_once-on-first-GetTypeID-
 * call pattern the other CF types use. */
__attribute__((constructor))
static void allocatorInit(void)
{
	static const CFRuntimeClass cls = {
		.className = "CFAllocator",
	};
	g_allocatorTypeID = _CFRuntimeRegisterClass(&cls);
	_CFRuntimeInitStaticInstance(&g_defaultAllocator, g_allocatorTypeID);
	g_currentDefault = &g_defaultAllocator;
}

CFTypeID CFAllocatorGetTypeID(void)
{
	return g_allocatorTypeID;
}

/* All four names alias the same malloc-backed singleton -- real objects
 * (not NULL), so CFGetTypeID()/CFEqual() on them behave normally, they
 * just all compare equal to each other and to CFAllocatorGetDefault(). */
const CFAllocatorRef kCFAllocatorDefault = (CFAllocatorRef)&g_defaultAllocator;
const CFAllocatorRef kCFAllocatorSystemDefault = (CFAllocatorRef)&g_defaultAllocator;
const CFAllocatorRef kCFAllocatorMalloc = (CFAllocatorRef)&g_defaultAllocator;
const CFAllocatorRef kCFAllocatorNull = (CFAllocatorRef)&g_defaultAllocator;

CFAllocatorRef CFAllocatorGetDefault(void)
{
	CFAllocatorGetTypeID();
	return g_currentDefault;
}

void CFAllocatorSetDefault(CFAllocatorRef allocator)
{
	CFAllocatorGetTypeID();
	g_currentDefault = allocator ? allocator : &g_defaultAllocator;
}

void *CFAllocatorAllocate(CFAllocatorRef allocator, CFIndex size, CFOptionFlags hint)
{
	(void)allocator;
	(void)hint;
	return malloc((size_t)size);
}

void *CFAllocatorReallocate(CFAllocatorRef allocator, void *ptr, CFIndex newsize, CFOptionFlags hint)
{
	(void)allocator;
	(void)hint;
	if (newsize == 0) {
		free(ptr);
		return NULL;
	}
	return realloc(ptr, (size_t)newsize);
}

void CFAllocatorDeallocate(CFAllocatorRef allocator, void *ptr)
{
	(void)allocator;
	free(ptr);
}
