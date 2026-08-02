/* Copyright (c) 2026 Vihaan Nathan
 *
 * Mach-O objc metadata parsing, class realization, and category
 * attachment. dyld hands us the mach headers of every loaded image
 * (already rebased/bound -- see userland/dyld/dyld_main.c's hardcoded
 * _objc_init hook) via objc_register_image(); this walks each image's
 * load commands looking for the __DATA,__objc_* sections clang emits.
 *
 * Two-pass registration, matching real objc: objc_register_image()
 * realizes every class up front (so any class can find any other by
 * name), and only after every currently-loading image has been through
 * that pass does objc_attach_categories() run -- letting a category in
 * one image extend a class defined in another.
 */
#include "objc_priv.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static Class g_classes[OBJC_MAX_CLASSES];
static int g_nclasses;

static struct protocol_t *g_protocols[OBJC_MAX_PROTOCOLS];
static int g_nprotocols;

#define MAX_PENDING_CATEGORIES 256
static struct category_t *g_pending_cats[MAX_PENDING_CATEGORIES];
static int g_npending_cats;

static uint64_t
image_slide(const struct mach_header_64 *mh)
{
	const uint8_t *cmd = (const uint8_t *)mh + sizeof(*mh);
	for (uint32_t i = 0; i < mh->ncmds; i++) {
		const struct load_command *lc = (const struct load_command *)cmd;
		if (lc->cmd == LC_SEGMENT_64) {
			const struct segment_command_64 *sc = (const struct segment_command_64 *)lc;
			if (sc->fileoff == 0 && sc->filesize > 0) {
				return (uint64_t)(uintptr_t)mh - sc->vmaddr;
			}
		}
		cmd += lc->cmdsize;
	}
	return 0;
}

/* Finds one __DATA,<sectname> section and returns its actual (slid)
 * address + element count for a given element size -- every objc
 * metadata list section is just a flat array of pointer-sized entries. */
static void *
find_section(const struct mach_header_64 *mh, uint64_t slide, const char *sectname, uint32_t *out_count)
{
	const uint8_t *cmd = (const uint8_t *)mh + sizeof(*mh);
	for (uint32_t i = 0; i < mh->ncmds; i++) {
		const struct load_command *lc = (const struct load_command *)cmd;
		if (lc->cmd == LC_SEGMENT_64) {
			const struct segment_command_64 *sc = (const struct segment_command_64 *)lc;
			const struct section_64 *secs = (const struct section_64 *)(sc + 1);
			for (uint32_t j = 0; j < sc->nsects; j++) {
				/* sectname is a fixed 16-byte field, not necessarily
				 * NUL-terminated (e.g. "__objc_classlist" and
				 * "__objc_protolist" are exactly 16 bytes) -- strcmp
				 * would read past it into the next section_64's own
				 * fields looking for a terminator that isn't there.
				 * strncmp bounded to 16 never reads past the field. */
				if (strncmp(secs[j].sectname, sectname, 16) == 0) {
					*out_count = (uint32_t)(secs[j].size / sizeof(void *));
					return (void *)(uintptr_t)(secs[j].addr + slide);
				}
			}
		}
		cmd += lc->cmdsize;
	}
	*out_count = 0;
	return 0;
}

/* Growable flattened lists inside class_rw_t -- categories can attach
 * after a class's own class_ro_t (compile-time, immutable) was emitted,
 * so the live method/protocol/property lists have to live somewhere
 * mutable. Realloc-based: simplest correct thing, and class/category
 * counts here are small enough that repeated doubling is not a
 * meaningful cost. */
void
rw_append_method(struct class_rw_t *rw, SEL name, const char *types, IMP imp)
{
	if (rw->method_count >= rw->method_cap) {
		rw->method_cap = rw->method_cap ? rw->method_cap * 2 : 4;
		rw->methods = realloc(rw->methods, sizeof(struct method_t) * rw->method_cap);
	}
	rw->methods[rw->method_count++] = (struct method_t){ .name = name, .types = types, .imp = imp };
}

void
rw_append_protocol(struct class_rw_t *rw, struct protocol_t *p)
{
	if (rw->protocol_count >= rw->protocol_cap) {
		rw->protocol_cap = rw->protocol_cap ? rw->protocol_cap * 2 : 4;
		rw->protocols = realloc(rw->protocols, sizeof(struct protocol_t *) * rw->protocol_cap);
	}
	rw->protocols[rw->protocol_count++] = p;
}

static void
rw_add_methods(struct class_rw_t *rw, struct method_list_t *ml)
{
	if (!ml || ml->count == 0) {
		return;
	}
	for (uint32_t i = 0; i < ml->count; i++) {
		struct method_t m = ml->first[i];
		SEL name = sel_registerName((const char *)(void *)m.name);
		rw_append_method(rw, name, m.types, m.imp);
	}
}

static void
rw_add_protocols(struct class_rw_t *rw, struct protocol_list_t *pl)
{
	if (!pl || pl->count == 0) {
		return;
	}
	for (uintptr_t i = 0; i < pl->count; i++) {
		rw_append_protocol(rw, pl->list[i]);
	}
}

static void
rw_add_properties(struct class_rw_t *rw, struct property_list_t *pl)
{
	if (!pl || pl->count == 0) {
		return;
	}
	int need = rw->property_count + (int)pl->count;
	if (need > rw->property_cap) {
		rw->property_cap = need;
		rw->properties = realloc(rw->properties, sizeof(struct property_t) * rw->property_cap);
	}
	for (uint32_t i = 0; i < pl->count; i++) {
		rw->properties[rw->property_count++] = pl->first[i];
	}
}

void
objc_register_dynamic_class(Class cls)
{
	if (g_nclasses >= OBJC_MAX_CLASSES) {
		fprintf(stderr, "libobjc: too many classes (max %d)\n", OBJC_MAX_CLASSES);
		abort();
	}
	g_classes[g_nclasses++] = cls;
}

/* Realizes one class_t: allocates its class_rw_t, flattens the compiled
 * base method/protocol/property lists (uniquing method names as we go --
 * clang emits method_t.name as a raw per-image string pointer, exactly
 * like a selref before fixup, so it needs the same treatment before
 * pointer-equality comparisons in dispatch.c are valid), and recurses
 * into the metaclass so class methods dispatch through the identical
 * mechanism. */
static void
realize_class(Class cls)
{
	struct class_t *ct = as_class_t(cls);
	if (!ct || class_rw(cls)) {
		return; /* already realized, or null */
	}

	/* The superclass must be realized (and ivar-patched) first: classlist
	 * order isn't guaranteed to be superclass-before-subclass, and often
	 * isn't across images (e.g. Animal/Dog are in the main executable's
	 * own classlist, realized before libobjc.A.dylib's Object is even
	 * looked at). Recursing here guarantees class_getInstanceSize(super)
	 * below is already correct regardless of registration order. */
	if (ct->superclass) {
		realize_class(ct->superclass);
	}

	struct class_ro_t *ro = ct->data; /* untagged: this class was never realized before */
	struct class_rw_t *rw = calloc(1, sizeof(*rw));
	rw->ro = ro;
	rw->realized = 1;

	/* Ivar offset patching (real nonfragile-ABI mechanism, not
	 * optional): clang bakes each ivar's offset into *ivar->offset at
	 * compile time using only what it can see in this translation unit
	 * -- it has no way to know the superclass's *actual* instance size
	 * once every image is linked and loaded, especially across images.
	 * Starting from the superclass's real (already-patched) size and
	 * re-laying out this class's own ivars here, patching the shared
	 * offset slot in place, is what makes -legs (a synthesized
	 * accessor reading through that same slot) address the right
	 * memory instead of silently landing wherever the compile-time
	 * guess put it. */
	uint32_t size = ct->superclass ? (uint32_t)class_getInstanceSize(ct->superclass) : 0;
	if (ro->ivars) {
		for (uint32_t i = 0; i < ro->ivars->count; i++) {
			struct ivar_t *iv = &ro->ivars->first[i];
			uint32_t align = 1u << iv->alignment;
			size = (size + align - 1) & ~(align - 1);
			*iv->offset = (int32_t)size;
			size += iv->size;
		}
	}
	rw->instance_size = size;

	class_set_rw(cls, rw); /* tags data so class_rw() recognizes this as realized -- see objc_priv.h */

	rw_add_methods(rw, ro->baseMethods);
	rw_add_protocols(rw, ro->baseProtocols);
	rw_add_properties(rw, ro->baseProperties);

	objc_register_dynamic_class(cls);

	if (ct->isa) {
		realize_class(ct->isa); /* metaclass */
	}
}

void
objc_register_image(const struct mach_header_64 *mh)
{
	uint64_t slide = image_slide(mh);

	uint32_t nselrefs = 0;
	SEL *selrefs = find_section(mh, slide, "__objc_selrefs", &nselrefs);
	for (uint32_t i = 0; i < nselrefs; i++) {
		objc_selref_fixup(&selrefs[i]);
	}

	uint32_t nclasses = 0;
	Class *classlist = find_section(mh, slide, "__objc_classlist", &nclasses);
	for (uint32_t i = 0; i < nclasses; i++) {
		realize_class(classlist[i]);
	}

	uint32_t ncats = 0;
	struct category_t **catlist = find_section(mh, slide, "__objc_catlist", &ncats);
	for (uint32_t i = 0; i < ncats; i++) {
		if (g_npending_cats < MAX_PENDING_CATEGORIES) {
			g_pending_cats[g_npending_cats++] = catlist[i];
		}
	}

	uint32_t nprotos = 0;
	struct protocol_t **protolist = find_section(mh, slide, "__objc_protolist", &nprotos);
	for (uint32_t i = 0; i < nprotos && g_nprotocols < OBJC_MAX_PROTOCOLS; i++) {
		g_protocols[g_nprotocols++] = protolist[i];
	}
}

void
objc_attach_categories(void)
{
	for (int i = 0; i < g_npending_cats; i++) {
		struct category_t *cat = g_pending_cats[i];
		Class cls = cat->cls;
		if (!cls) {
			continue;
		}
		struct class_rw_t *rw = class_rw(cls);
		if (rw) {
			rw_add_methods(rw, cat->instanceMethods);
			rw_add_protocols(rw, cat->protocols);
			rw_add_properties(rw, cat->instanceProperties);
		}
		struct class_rw_t *meta_rw = class_rw((Class)as_class_t(cls)->isa);
		if (meta_rw) {
			rw_add_methods(meta_rw, cat->classMethods);
		}
	}
	g_npending_cats = 0;
}

Class
objc_getClass(const char *name)
{
	if (!name) {
		return Nil;
	}
	for (int i = 0; i < g_nclasses; i++) {
		if (strcmp(class_getName(g_classes[i]), name) == 0) {
			return g_classes[i];
		}
	}
	return Nil;
}

Class
class_lookup_or_panic(const char *name)
{
	Class c = objc_getClass(name);
	if (!c) {
		fprintf(stderr, "libobjc: class %s not found\n", name);
		abort();
	}
	return c;
}

const char *
class_getName(Class cls)
{
	if (!cls) {
		return "nil";
	}
	struct class_rw_t *rw = class_rw(cls);
	return rw ? rw->ro->name : as_class_t(cls)->data->name;
}

Class
class_getSuperclass(Class cls)
{
	return cls ? (Class)as_class_t(cls)->superclass : Nil;
}

BOOL
class_isMetaClass(Class cls)
{
	if (!cls) {
		return NO;
	}
	struct class_rw_t *rw = class_rw(cls);
	uint32_t flags = rw ? rw->ro->flags : as_class_t(cls)->data->flags;
	return (flags & RO_META) ? YES : NO;
}

size_t
class_getInstanceSize(Class cls)
{
	if (!cls) {
		return 0;
	}
	struct class_rw_t *rw = class_rw(cls);
	return rw ? rw->instance_size : 0;
}

struct method_t *
class_lookup_method_t(Class cls, SEL sel)
{
	for (Class c = cls; c; c = (Class)as_class_t(c)->superclass) {
		struct class_rw_t *rw = class_rw(c);
		if (!rw) {
			continue;
		}
		for (int i = 0; i < rw->method_count; i++) {
			if (rw->methods[i].name == sel) {
				return &rw->methods[i];
			}
		}
	}
	return 0;
}

Method
class_getInstanceMethod(Class cls, SEL name)
{
	return (Method)class_lookup_method_t(cls, name);
}

IMP
class_getMethodImplementation(Class cls, SEL name)
{
	struct method_t *m = class_lookup_method_t(cls, name);
	return m ? m->imp : 0;
}

BOOL
class_respondsToSelector(Class cls, SEL sel)
{
	return class_lookup_method_t(cls, sel) ? YES : NO;
}

BOOL
class_conformsToProtocol(Class cls, Protocol *proto)
{
	if (!cls || !proto) {
		return NO;
	}
	struct protocol_t *target = (struct protocol_t *)(void *)proto;
	for (Class c = cls; c; c = (Class)as_class_t(c)->superclass) {
		struct class_rw_t *rw = class_rw(c);
		if (!rw) {
			continue;
		}
		for (int i = 0; i < rw->protocol_count; i++) {
			if (rw->protocols[i] == target ||
			    (rw->protocols[i]->name && target->name &&
			     strcmp(rw->protocols[i]->name, target->name) == 0)) {
				return YES;
			}
		}
	}
	return NO;
}

static struct ivar_t *
find_ivar(Class cls, const char *name)
{
	struct class_rw_t *rw = class_rw(cls);
	if (!rw) {
		return 0;
	}
	if (rw->ro->ivars) {
		for (uint32_t i = 0; i < rw->ro->ivars->count; i++) {
			if (strcmp(rw->ro->ivars->first[i].name, name) == 0) {
				return &rw->ro->ivars->first[i];
			}
		}
	}
	for (int i = 0; i < rw->extra_ivar_count; i++) {
		if (strcmp(rw->extra_ivars[i].name, name) == 0) {
			return &rw->extra_ivars[i];
		}
	}
	return 0;
}

Ivar
class_getInstanceVariable(Class cls, const char *name)
{
	for (Class c = cls; c; c = (Class)as_class_t(c)->superclass) {
		struct ivar_t *iv = find_ivar(c, name);
		if (iv) {
			return (Ivar)iv;
		}
	}
	return 0;
}

Ivar *
class_copyIvarList(Class cls, unsigned int *outCount)
{
	struct class_rw_t *rw = class_rw(cls);
	int n = 0;
	if (rw) {
		if (rw->ro->ivars) {
			n += (int)rw->ro->ivars->count;
		}
		n += rw->extra_ivar_count;
	}
	if (outCount) {
		*outCount = (unsigned int)n;
	}
	if (n == 0) {
		return 0;
	}
	Ivar *out = malloc(sizeof(Ivar) * n);
	int k = 0;
	if (rw->ro->ivars) {
		for (uint32_t i = 0; i < rw->ro->ivars->count; i++) {
			out[k++] = (Ivar)&rw->ro->ivars->first[i];
		}
	}
	for (int i = 0; i < rw->extra_ivar_count; i++) {
		out[k++] = (Ivar)&rw->extra_ivars[i];
	}
	return out;
}

const char *
ivar_getName(Ivar ivar)
{
	return ivar ? ((struct ivar_t *)ivar)->name : 0;
}

ptrdiff_t
ivar_getOffset(Ivar ivar)
{
	return ivar ? *((struct ivar_t *)ivar)->offset : 0;
}

Class
object_getClass(id obj)
{
	return obj ? obj->isa : Nil;
}

Class
object_setClass(id obj, Class cls)
{
	if (!obj) {
		return Nil;
	}
	Class old = obj->isa;
	obj->isa = cls;
	return old;
}

const char *
object_getClassName(id obj)
{
	return obj ? class_getName(obj->isa) : "nil";
}

id
object_getIvar(id obj, Ivar ivar)
{
	if (!obj || !ivar) {
		return nil;
	}
	struct ivar_t *iv = (struct ivar_t *)ivar;
	return *(id *)((char *)obj + *iv->offset);
}

void
object_setIvar(id obj, Ivar ivar, id value)
{
	if (!obj || !ivar) {
		return;
	}
	struct ivar_t *iv = (struct ivar_t *)ivar;
	*(id *)((char *)obj + *iv->offset) = value;
}

Ivar
object_setInstanceVariable(id obj, const char *name, void *value)
{
	Ivar iv = class_getInstanceVariable(obj->isa, name);
	if (iv) {
		object_setIvar(obj, iv, (id)value);
	}
	return iv;
}

Ivar
object_getInstanceVariable(id obj, const char *name, void **outValue)
{
	Ivar iv = class_getInstanceVariable(obj->isa, name);
	if (iv && outValue) {
		*outValue = (void *)object_getIvar(obj, iv);
	}
	return iv;
}

SEL
method_getName(Method m)
{
	return m ? ((struct method_t *)m)->name : 0;
}

IMP
method_getImplementation(Method m)
{
	return m ? ((struct method_t *)m)->imp : 0;
}

const char *
method_getTypeEncoding(Method m)
{
	return m ? ((struct method_t *)m)->types : 0;
}
