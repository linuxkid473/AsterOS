/* Copyright (c) 2026 Vihaan Nathan
 *
 * Object: a minimal NSObject-equivalent root class, real Objective-C
 * compiled and registered through the exact same path any other class
 * takes (see class.c) -- not hand-authored metadata. Deliberately
 * written in Objective-C, not C: its own method bodies exercise
 * objc_msgSend/class registration the same way client code will,
 * making this file itself part of the ground truth this runtime is
 * checked against.
 *
 * Compiled without ARC (-fno-objc-arc, see build.sh): this class
 * defines what retain/release/dealloc *mean*, so it manages its own
 * memory manually, calling straight into the _objc_root* C helpers
 * arc.c's ARC-facing objc_retain/objc_release/objc_autorelease message-
 * send through to. No Foundation yet, so -description returns a plain
 * C string rather than NSString* -- documented deviation, not an
 * oversight (nothing could depend on the real signature without
 * Foundation existing to begin with).
 */
#include <objc/objc.h>
#include <objc/runtime.h>
#include <stdint.h>

extern id _objc_rootAlloc(Class cls);
extern id _objc_rootInit(id obj);
extern void _objc_rootDealloc(id obj);
extern id _objc_rootRetain(id obj);
extern void _objc_rootRelease(id obj);
extern id _objc_rootAutorelease(id obj);
extern uintptr_t _objc_rootRetainCount(id obj);
extern Class _objc_rootClass(id obj);
extern BOOL _objc_rootIsKindOfClass(id obj, Class cls);
extern BOOL _objc_rootRespondsToSelector(id obj, SEL sel);

__attribute__((objc_root_class))
@interface Object
{
	Class isa;
}
@end

@implementation Object

+ (id)alloc
{
	return _objc_rootAlloc(self);
}

+ (id)new
{
	return [[self alloc] init];
}

- (id)init
{
	return _objc_rootInit(self);
}

- (void)dealloc
{
	_objc_rootDealloc(self);
}

- (id)retain
{
	return _objc_rootRetain(self);
}

- (void)release
{
	_objc_rootRelease(self);
}

- (id)autorelease
{
	return _objc_rootAutorelease(self);
}

- (uintptr_t)retainCount
{
	return _objc_rootRetainCount(self);
}

+ (Class)class
{
	return self;
}

- (Class)class
{
	return _objc_rootClass(self);
}

+ (Class)superclass
{
	return class_getSuperclass(self);
}

- (BOOL)isKindOfClass:(Class)cls
{
	return _objc_rootIsKindOfClass(self, cls);
}

- (BOOL)isMemberOfClass:(Class)cls
{
	return _objc_rootClass(self) == cls;
}

- (BOOL)respondsToSelector:(SEL)sel
{
	return _objc_rootRespondsToSelector(self, sel);
}

- (BOOL)conformsToProtocol:(Protocol *)proto
{
	return class_conformsToProtocol(_objc_rootClass(self), proto);
}

+ (BOOL)respondsToSelector:(SEL)sel
{
	return class_respondsToSelector((Class)self, sel);
}

- (BOOL)isEqual:(id)other
{
	return self == other;
}

- (uintptr_t)hash
{
	return (uintptr_t)self;
}

- (id)self
{
	return self;
}

- (const char *)description
{
	return class_getName(_objc_rootClass(self));
}

@end
