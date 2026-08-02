/* Copyright (c) 2026 Vihaan Nathan
 *
 * On-disk Objective-C metadata layout: the structures clang actually
 * emits into __DATA,__objc_const / __objc_data / __data for
 * -target x86_64-apple-macos10.15 -fobjc-runtime=macosx (nonfragile
 * ABI2, absolute/non-relative method lists -- the relative-pointer
 * method_t variant is an arm64e-era optimization this target doesn't
 * use). Every field here was ground-truthed against a real compiled
 * probe .o (otool -o / otool -rv / raw hex dump), not written from
 * memory -- see TODO.md Phase 13 for the probe program. These structs
 * are read-only input to this runtime; our own live bookkeeping
 * (objc_priv.h) is a completely separate, non-ABI-constrained layout.
 */
#ifndef OBJC_ABI_H
#define OBJC_ABI_H

#include <stdint.h>
#include <objc/objc.h>

struct method_t {
	SEL name;
	const char *types;
	IMP imp;
};

struct method_list_t {
	uint32_t entsize;
	uint32_t count;
	struct method_t first[];
};

struct ivar_t {
	int32_t *offset;
	const char *name;
	const char *type;
	uint32_t alignment;
	uint32_t size;
};

struct ivar_list_t {
	uint32_t entsize;
	uint32_t count;
	struct ivar_t first[];
};

struct property_t {
	const char *name;
	const char *attributes;
};

struct property_list_t {
	uint32_t entsize;
	uint32_t count;
	struct property_t first[];
};

struct protocol_list_t {
	uintptr_t count;
	struct protocol_t *list[];
};

/* `size` gates which trailing fields are actually present on disk --
 * clang has grown this struct across OS releases. Everything through
 * instanceProperties has always been present; check `size` before
 * touching anything after it. */
struct protocol_t {
	Class isa;
	const char *name;
	struct protocol_list_t *protocols;
	struct method_list_t *instanceMethods;
	struct method_list_t *classMethods;
	struct method_list_t *optionalInstanceMethods;
	struct method_list_t *optionalClassMethods;
	struct property_list_t *instanceProperties;
	uint32_t size;
	uint32_t flags;
	const char **extendedMethodTypes;
	const char *demangledName;
	struct property_list_t *classProperties;
};

struct category_t {
	const char *name;
	Class cls;
	struct method_list_t *instanceMethods;
	struct method_list_t *classMethods;
	struct protocol_list_t *protocols;
	struct property_list_t *instanceProperties;
};

#define RO_META 0x1
#define RO_ROOT 0x2

struct class_ro_t {
	uint32_t flags;
	uint32_t instanceStart;
	uint32_t instanceSize;
	uint32_t reserved;
	const uint8_t *ivarLayout;
	const char *name;
	struct method_list_t *baseMethods;
	struct protocol_list_t *baseProtocols;
	struct ivar_list_t *ivars;
	const uint8_t *weakIvarLayout;
	struct property_list_t *baseProperties;
};

/* class_t.cache/vtable are always zero as emitted by the compiler --
 * cache is populated by this runtime at realization time (our own
 * bucket format, see objc_priv.h), vtable is an unused legacy slot on
 * this ABI generation. class_t.data, as emitted, is a plain untagged
 * pointer to class_ro_t (ground-truthed: raw section bytes are zero
 * before relocation, i.e. no low-bit tag baked in) -- this runtime
 * overwrites it with a pointer to our own class_rw_t at realization
 * time, which is safe precisely because nothing outside this runtime
 * ever reads a live class_t.data again. */
struct class_t {
	Class isa;
	Class superclass;
	void *cache;
	void *vtable;
	struct class_ro_t *data;
};

#endif /* OBJC_ABI_H */
