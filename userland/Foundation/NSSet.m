/* Copyright (c) 2026 Vihaan Nathan -- see NSSet.h */
#import <Foundation/Foundation.h>
#include "NSCFBridge.h"
#include <stdlib.h>

@interface NSCFSet : NSMutableSet
@end

@implementation NSSet

+ (id)alloc
{
	return (id)CFSetCreateMutable(kCFAllocatorDefault, 0, &kNSObjectSetCallBacks);
}

+ (instancetype)set
{
	return [[[NSCFSet alloc] init] autorelease];
}

+ (instancetype)setWithObject:(id)anObject
{
	return [[[NSCFSet alloc] initWithObjects:&anObject count:1] autorelease];
}

+ (instancetype)setWithObjects:(const id [])objects count:(NSUInteger)count
{
	return [[[NSCFSet alloc] initWithObjects:objects count:count] autorelease];
}

+ (instancetype)setWithSet:(NSSet *)set
{
	return [[[NSCFSet alloc] initWithSet:set] autorelease];
}

- (instancetype)init
{
	return self;
}

- (instancetype)initWithObjects:(const id [])objects count:(NSUInteger)count
{
	[self release];
	return (id)CFSetCreate(kCFAllocatorDefault, (const void **)objects, (CFIndex)count, &kNSObjectSetCallBacks);
}

- (instancetype)initWithSet:(NSSet *)set
{
	[self release];
	return (id)CFSetCreateCopy(kCFAllocatorDefault, (CFSetRef)set);
}

- (id)copyWithZone:(NSZone *)zone
{
	(void)zone;
	return (id)CFSetCreateCopy(kCFAllocatorDefault, (CFSetRef)self);
}

- (id)mutableCopyWithZone:(NSZone *)zone
{
	(void)zone;
	return (id)CFSetCreateMutableCopy(kCFAllocatorDefault, 0, (CFSetRef)self);
}

@end

@implementation NSMutableSet

+ (instancetype)setWithCapacity:(NSUInteger)capacity
{
	return [[[NSCFSet alloc] initWithCapacity:capacity] autorelease];
}

- (instancetype)initWithCapacity:(NSUInteger)capacity
{
	[self release];
	return (id)CFSetCreateMutable(kCFAllocatorDefault, (CFIndex)capacity, &kNSObjectSetCallBacks);
}

@end

@implementation NSCFSet

- (id)retain { return NSCFBridge_retain(self); }
- (oneway void)release { NSCFBridge_release(self); }
- (NSUInteger)retainCount { return NSCFBridge_retainCount(self); }
- (BOOL)isEqual:(id)other { return NSCFBridge_isEqual(self, other); }
- (NSUInteger)hash { return NSCFBridge_hash(self); }
- (NSString *)description { return NSCFBridge_description(self); }
- (void)dealloc { NSCFBridge_deallocGuard(self); }

- (NSUInteger)count
{
	return (NSUInteger)CFSetGetCount((CFSetRef)self);
}

- (BOOL)containsObject:(id)anObject
{
	return CFSetContainsValue((CFSetRef)self, (const void *)anObject) ? YES : NO;
}

- (id)anyObject
{
	if (CFSetGetCount((CFSetRef)self) == 0) {
		return nil;
	}
	const void *v = NULL;
	CFSetGetValues((CFSetRef)self, &v);
	return (id)v;
}

- (NSArray *)allObjects
{
	CFIndex c = CFSetGetCount((CFSetRef)self);
	if (c == 0) {
		return [NSArray array];
	}
	const void **values = malloc(sizeof(const void *) * (size_t)c);
	CFSetGetValues((CFSetRef)self, values);
	NSArray *result = [NSArray arrayWithObjects:(const id *)values count:(NSUInteger)c];
	free(values);
	return result;
}

- (void)addObject:(id)anObject
{
	CFSetAddValue((CFMutableSetRef)self, (const void *)anObject);
}

- (void)removeObject:(id)anObject
{
	CFSetRemoveValue((CFMutableSetRef)self, (const void *)anObject);
}

- (void)removeAllObjects
{
	CFSetRemoveAllValues((CFMutableSetRef)self);
}

@end
