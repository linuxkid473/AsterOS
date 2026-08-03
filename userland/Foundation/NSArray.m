/* Copyright (c) 2026 Vihaan Nathan -- see NSArray.h. Same one-concrete-
 * class-per-cluster pattern as NSString.m; see that file's header
 * comment for the +alloc/-initWith...: placeholder-replacement design.
 */
#import <Foundation/Foundation.h>
#include "NSCFBridge.h"

@interface NSCFArray : NSMutableArray
@end

@implementation NSArray

+ (id)alloc
{
	return (id)CFArrayCreateMutable(kCFAllocatorDefault, 0, &kNSObjectArrayCallBacks);
}

+ (instancetype)array
{
	return [[[NSCFArray alloc] init] autorelease];
}

+ (instancetype)arrayWithObject:(id)anObject
{
	return [[[NSCFArray alloc] initWithObjects:&anObject count:1] autorelease];
}

+ (instancetype)arrayWithObjects:(const id [])objects count:(NSUInteger)count
{
	return [[[NSCFArray alloc] initWithObjects:objects count:count] autorelease];
}

+ (instancetype)arrayWithArray:(NSArray *)array
{
	return [[[NSCFArray alloc] initWithArray:array] autorelease];
}

- (instancetype)init
{
	return self;
}

- (instancetype)initWithObjects:(const id [])objects count:(NSUInteger)count
{
	[self release];
	return (id)CFArrayCreate(kCFAllocatorDefault, (const void **)objects, (CFIndex)count, &kNSObjectArrayCallBacks);
}

- (instancetype)initWithArray:(NSArray *)array
{
	[self release];
	return (id)CFArrayCreateCopy(kCFAllocatorDefault, (CFArrayRef)array);
}

- (id)copyWithZone:(NSZone *)zone
{
	(void)zone;
	return (id)CFArrayCreateCopy(kCFAllocatorDefault, (CFArrayRef)self);
}

- (id)mutableCopyWithZone:(NSZone *)zone
{
	(void)zone;
	return (id)CFArrayCreateMutableCopy(kCFAllocatorDefault, 0, (CFArrayRef)self);
}

- (id)objectAtIndexedSubscript:(NSUInteger)idx
{
	return [self objectAtIndex:idx];
}

- (NSArray *)arrayByAddingObject:(id)anObject
{
	NSMutableArray *m = [[self mutableCopy] autorelease];
	[m addObject:anObject];
	return m;
}

- (NSString *)componentsJoinedByString:(NSString *)separator
{
	return (id)CFStringCreateByCombiningStrings(kCFAllocatorDefault, (CFArrayRef)self, (CFStringRef)separator);
}

@end

@implementation NSMutableArray

+ (instancetype)arrayWithCapacity:(NSUInteger)capacity
{
	return [[[NSCFArray alloc] initWithCapacity:capacity] autorelease];
}

- (instancetype)initWithCapacity:(NSUInteger)capacity
{
	[self release];
	return (id)CFArrayCreateMutable(kCFAllocatorDefault, (CFIndex)capacity, &kNSObjectArrayCallBacks);
}

@end

@implementation NSCFArray

- (id)retain { return NSCFBridge_retain(self); }
- (oneway void)release { NSCFBridge_release(self); }
- (NSUInteger)retainCount { return NSCFBridge_retainCount(self); }
- (BOOL)isEqual:(id)other { return NSCFBridge_isEqual(self, other); }
- (NSUInteger)hash { return NSCFBridge_hash(self); }
- (NSString *)description { return NSCFBridge_description(self); }
- (void)dealloc { NSCFBridge_deallocGuard(self); }

- (NSUInteger)count
{
	return (NSUInteger)CFArrayGetCount((CFArrayRef)self);
}

- (id)objectAtIndex:(NSUInteger)index
{
	return (id)CFArrayGetValueAtIndex((CFArrayRef)self, (CFIndex)index);
}

- (id)firstObject
{
	return [self count] ? [self objectAtIndex:0] : nil;
}

- (id)lastObject
{
	NSUInteger c = [self count];
	return c ? [self objectAtIndex:c - 1] : nil;
}

- (BOOL)containsObject:(id)anObject
{
	CFIndex c = CFArrayGetCount((CFArrayRef)self);
	return CFArrayContainsValue((CFArrayRef)self, CFRangeMake(0, c), (const void *)anObject) ? YES : NO;
}

- (NSUInteger)indexOfObject:(id)anObject
{
	CFIndex c = CFArrayGetCount((CFArrayRef)self);
	CFIndex i = CFArrayGetFirstIndexOfValue((CFArrayRef)self, CFRangeMake(0, c), (const void *)anObject);
	return i == -1 ? NSIntegerMax : (NSUInteger)i;
}

- (void)addObject:(id)anObject
{
	CFArrayAppendValue((CFMutableArrayRef)self, (const void *)anObject);
}

- (void)insertObject:(id)anObject atIndex:(NSUInteger)index
{
	CFArrayInsertValueAtIndex((CFMutableArrayRef)self, (CFIndex)index, (const void *)anObject);
}

- (void)removeObjectAtIndex:(NSUInteger)index
{
	CFArrayRemoveValueAtIndex((CFMutableArrayRef)self, (CFIndex)index);
}

- (void)removeObject:(id)anObject
{
	CFIndex c = CFArrayGetCount((CFArrayRef)self);
	CFIndex i = CFArrayGetFirstIndexOfValue((CFArrayRef)self, CFRangeMake(0, c), (const void *)anObject);
	if (i != -1) {
		CFArrayRemoveValueAtIndex((CFMutableArrayRef)self, i);
	}
}

- (void)removeAllObjects
{
	CFArrayRemoveAllValues((CFMutableArrayRef)self);
}

- (void)replaceObjectAtIndex:(NSUInteger)index withObject:(id)anObject
{
	CFArraySetValueAtIndex((CFMutableArrayRef)self, (CFIndex)index, (const void *)anObject);
}

- (void)addObjectsFromArray:(NSArray *)otherArray
{
	CFIndex c = CFArrayGetCount((CFArrayRef)otherArray);
	CFArrayAppendArray((CFMutableArrayRef)self, (CFArrayRef)otherArray, CFRangeMake(0, c));
}

@end
