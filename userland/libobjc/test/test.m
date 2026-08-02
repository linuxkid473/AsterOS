/* Copyright (c) 2026 Vihaan Nathan
 *
 * Real Objective-C, compiled unmodified with the host's off-the-shelf
 * clang (-fobjc-arc -fobjc-runtime=macosx, same target as everything
 * else in this project) -- the actual Phase 13 regression test.
 * Exercises: a subclass with an ivar + a synthesized property, a
 * category, protocol conformance, [super ...], ARC-managed alloc/
 * release via scope exit, and an explicit autorelease pool.
 */
#include <objc/objc.h>
#include <objc/runtime.h>
#include <stdio.h>

@interface Object
+ (id)alloc;
+ (id)new;
- (id)init;
- (void)dealloc;
- (id)autorelease;
- (Class)class;
- (BOOL)isKindOfClass:(Class)cls;
- (BOOL)respondsToSelector:(SEL)sel;
- (BOOL)conformsToProtocol:(Protocol *)proto;
- (const char *)description;
@end

/* ARC forbids explicit -autorelease *message sends* (ARC decides when to
 * autorelease on its own). Rather than fight ARC's cooperative
 * retain/autorelease fast-path protocol from inside this same
 * ARC-compiled translation unit (ground-truthed to be genuinely fragile
 * here -- see TODO.md Phase 13), the actual -autorelease call lives in
 * mrc_helper.m, its own non-ARC translation unit; calling it as an
 * ordinary extern C function gets none of ARC's message-send-adjacent
 * bracketing. */
extern id test_mrc_autorelease(id obj);

@protocol Greeter
- (const char *)greeting;
@end

@interface Animal : Object
{
	int _legs;
}
@property (nonatomic) int legs;
- (const char *)speak;
@end

@implementation Animal
@synthesize legs = _legs;

- (id)init
{
	self = [super init];
	if (self) {
		_legs = 4;
	}
	return self;
}

- (const char *)speak
{
	return "...";
}
@end

@interface Animal (Description) <Greeter>
@end

@implementation Animal (Description)
- (const char *)greeting
{
	return "hello from a category";
}
@end

@interface Dog : Animal
@end

@implementation Dog
- (const char *)speak
{
	return "Woof!";
}
@end

static int g_dealloc_count;

@interface Counter : Object
@end

@implementation Counter
- (void)dealloc
{
	g_dealloc_count++;
	/* ARC auto-inserts the call to super's -dealloc after this method
	 * body runs -- writing it explicitly is a compile error under ARC. */
}
@end

int
main(void)
{
	int failures = 0;

	Dog *d = [[Dog alloc] init];
	if (d.legs != 4) {
		printf("OBJCTEST FAIL: property/ivar via inherited init wrong: %d\n", d.legs);
		failures++;
	}
	if (![d isKindOfClass:[Animal class]]) {
		printf("OBJCTEST FAIL: isKindOfClass wrong\n");
		failures++;
	}
	const char *bark = [d speak];
	if (bark[0] != 'W') {
		printf("OBJCTEST FAIL: overridden -speak wrong: %s\n", bark);
		failures++;
	}
	if (![d respondsToSelector:@selector(greeting)]) {
		printf("OBJCTEST FAIL: category method not attached\n");
		failures++;
	} else {
		const char *g = [d greeting];
		if (g[0] != 'h') {
			printf("OBJCTEST FAIL: category method wrong result: %s\n", g);
			failures++;
		}
	}
	if (![d conformsToProtocol:@protocol(Greeter)]) {
		printf("OBJCTEST FAIL: protocol conformance via category not recognized\n");
		failures++;
	}

	@autoreleasepool {
		Counter *tmp = [[Counter alloc] init];
		test_mrc_autorelease(tmp);
	}
	if (g_dealloc_count != 1) {
		printf("OBJCTEST FAIL: autorelease pool drain didn't dealloc (count=%d)\n", g_dealloc_count);
		failures++;
	}

	{
		Counter *c2 = [[Counter alloc] init];
		c2 = nil; /* ARC releases the last strong ref on reassignment */
	}
	if (g_dealloc_count != 2) {
		printf("OBJCTEST FAIL: ARC scope release didn't dealloc (count=%d)\n", g_dealloc_count);
		failures++;
	}

	if (failures == 0) {
		printf("OBJCTEST PASS\n");
	}
	return failures;
}
