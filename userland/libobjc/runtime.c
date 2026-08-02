/* Copyright (c) 2026 Vihaan Nathan
 *
 * Dynamic class creation: objc_allocateClassPair/objc_registerClassPair
 * and the class_add* mutators that run in between them. Unlike a
 * compiled class (class.c's realize_class, working from an immutable
 * clang-emitted class_ro_t), a class created this way owns its
 * class_ro_t outright -- there's no image data backing it, so it's
 * safe for class_addIvar to grow instanceSize in place.
 */
#include "objc_priv.h"
#include <string.h>
#include <stdlib.h>

Class
objc_allocateClassPair(Class superclass, const char *name, size_t extraBytes)
{
	if (objc_getClass(name)) {
		return Nil; /* name already in use, matches Apple's documented behavior */
	}

	struct class_t *cls = calloc(1, sizeof(struct class_t));
	struct class_t *meta = calloc(1, sizeof(struct class_t));
	struct class_rw_t *cls_rw = calloc(1, sizeof(struct class_rw_t));
	struct class_rw_t *meta_rw = calloc(1, sizeof(struct class_rw_t));
	struct class_ro_t *cls_ro = calloc(1, sizeof(struct class_ro_t));
	struct class_ro_t *meta_ro = calloc(1, sizeof(struct class_ro_t));

	char *name_copy = malloc(strlen(name) + 1);
	memcpy(name_copy, name, strlen(name) + 1);

	cls_ro->name = name_copy;
	meta_ro->name = name_copy;
	meta_ro->flags = RO_META;

	cls->isa = (Class)meta;
	cls->superclass = superclass;
	cls->data = cls_ro;

	meta->isa = superclass ? as_class_t(superclass)->isa : (Class)meta;
	meta->superclass = superclass ? as_class_t(superclass)->isa : Nil;
	meta->data = meta_ro;

	cls_rw->ro = cls_ro;
	cls_rw->realized = 1;
	cls_rw->instance_size = superclass ? (uint32_t)class_getInstanceSize(superclass) : sizeof(struct objc_object);
	cls_rw->instance_size += (uint32_t)extraBytes;
	class_set_rw((Class)cls, cls_rw);

	meta_rw->ro = meta_ro;
	meta_rw->realized = 1;
	class_set_rw((Class)meta, meta_rw);

	/* Not registered in the name table yet -- objc_registerClassPair
	 * does that, matching Apple's documented two-step API (class_add*
	 * is only valid on the unregistered pair). */
	return (Class)cls;
}

void
objc_registerClassPair(Class cls)
{
	objc_register_dynamic_class(cls);
}

BOOL
class_addMethod(Class cls, SEL name, IMP imp, const char *types)
{
	struct class_rw_t *rw = class_rw(cls);
	if (!rw || class_lookup_method_t(cls, name)) {
		return NO;
	}
	rw_append_method(rw, name, types, imp);
	return YES;
}

BOOL
class_addProtocol(Class cls, Protocol *proto)
{
	struct class_rw_t *rw = class_rw(cls);
	if (!rw) {
		return NO;
	}
	rw_append_protocol(rw, (struct protocol_t *)(void *)proto);
	return YES;
}

BOOL
class_addIvar(Class cls, const char *name, size_t size, uint8_t alignment, const char *types)
{
	struct class_rw_t *rw = class_rw(cls);
	if (!rw) {
		return NO;
	}

	uint32_t align_bytes = 1u << alignment;
	uint32_t offset = (rw->instance_size + align_bytes - 1) & ~(align_bytes - 1);

	if (rw->extra_ivar_count >= rw->extra_ivar_cap) {
		rw->extra_ivar_cap = rw->extra_ivar_cap ? rw->extra_ivar_cap * 2 : 4;
		rw->extra_ivars = realloc(rw->extra_ivars, sizeof(struct ivar_t) * rw->extra_ivar_cap);
	}

	int32_t *offset_storage = malloc(sizeof(int32_t));
	*offset_storage = (int32_t)offset;

	char *name_copy = malloc(strlen(name) + 1);
	memcpy(name_copy, name, strlen(name) + 1);

	rw->extra_ivars[rw->extra_ivar_count++] = (struct ivar_t){
		.offset = offset_storage,
		.name = name_copy,
		.type = types,
		.alignment = alignment,
		.size = (uint32_t)size,
	};
	rw->instance_size = offset + (uint32_t)size;
	return YES;
}

id
class_createInstance(Class cls, size_t extraBytes)
{
	if (!cls) {
		return nil;
	}
	size_t size = class_getInstanceSize(cls) + extraBytes;
	id obj = calloc(1, size);
	if (obj) {
		obj->isa = cls;
	}
	return obj;
}
