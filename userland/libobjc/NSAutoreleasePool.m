/* Copyright (c) 2026 Vihaan Nathan
 *
 * A minimal real NSAutoreleasePool, wrapping objc_autoreleasePoolPush/
 * Pop (autorelease.c). Not just a placeholder: clang's @autoreleasepool
 * codegen unconditionally emits a __DATA,__objc_classrefs entry for
 * _OBJC_CLASS_$_NSAutoreleasePool as compatibility scaffolding (never
 * actually loaded/dereferenced under the modern runtime -- confirmed by
 * inspecting the generated assembly, see TODO.md Phase 13), which fails
 * to link without something providing the symbol. Since Foundation
 * doesn't exist yet, this also happens to be a genuinely useful class on
 * its own for the legacy [[NSAutoreleasePool alloc] init] / [pool
 * release] pattern, not just link-time filler.
 */
#include <objc/objc.h>
#include <objc/runtime.h>

extern void *objc_autoreleasePoolPush(void);
extern void objc_autoreleasePoolPop(void *token);

@interface Object
+ (id)alloc;
- (id)init;
- (void)dealloc;
- (void)release;
@end

@interface NSAutoreleasePool : Object
{
	void *_token;
}
- (void)release;
- (void)drain;
@end

@implementation NSAutoreleasePool

- (id)init
{
	self = [super init];
	if (self) {
		_token = objc_autoreleasePoolPush();
	}
	return self;
}

- (void)release
{
	if (_token) {
		objc_autoreleasePoolPop(_token);
		_token = 0;
	}
	[super release];
}

/* -drain: real Darwin's NSAutoreleasePool has both -release and -drain --
 * historically -drain also triggered a GC collection under
 * garbage-collected Foundation, but under plain refcounting (all this
 * runtime supports) the two are identical. clang's @autoreleasepool
 * codegen sends -drain, not -release, for compatibility reasons ground-
 * truthed empirically (see TODO.md Phase 13). */
- (void)drain
{
	[self release];
}

- (void)dealloc
{
	if (_token) {
		objc_autoreleasePoolPop(_token);
	}
	[super dealloc];
}

@end
