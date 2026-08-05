/* Copyright (c) 2026 Vihaan Nathan
 *
 * Shared refcounting base every dispatch_*_create() builds on.
 */
#include "dispatch_internal.h"
#include <stdlib.h>

void
_dispatch_object_init(struct dispatch_object_hdr *hdr, void (*destroy)(void *self))
{
	hdr->refcount = 1;
	hdr->context = NULL;
	hdr->destroy = destroy;
}

void
dispatch_retain(dispatch_object_t object)
{
	struct dispatch_object_hdr *hdr = object;
	__atomic_add_fetch(&hdr->refcount, 1, __ATOMIC_RELAXED);
}

void
dispatch_release(dispatch_object_t object)
{
	struct dispatch_object_hdr *hdr = object;
	if (__atomic_sub_fetch(&hdr->refcount, 1, __ATOMIC_ACQ_REL) == 0) {
		hdr->destroy(object);
	}
}

void *
dispatch_get_context(dispatch_object_t object)
{
	struct dispatch_object_hdr *hdr = object;
	return hdr->context;
}

void
dispatch_set_context(dispatch_object_t object, void *context)
{
	struct dispatch_object_hdr *hdr = object;
	hdr->context = context;
}
