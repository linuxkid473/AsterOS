/* Copyright (c) 2026 Vihaan Nathan -- see NSDictionary.h */
#import <Foundation/Foundation.h>
#include "NSCFBridge.h"
#include <stdlib.h>

@interface NSCFDictionary : NSMutableDictionary
@end

@implementation NSDictionary

+ (id)alloc
{
	return (id)CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kNSObjectDictionaryKeyCallBacks, &kNSObjectDictionaryValueCallBacks);
}

+ (instancetype)dictionary
{
	return [[[NSCFDictionary alloc] init] autorelease];
}

+ (instancetype)dictionaryWithObject:(id)object forKey:(id)key
{
	return [[[NSCFDictionary alloc] initWithObjects:&object forKeys:&key count:1] autorelease];
}

+ (instancetype)dictionaryWithObjects:(const id [])objects forKeys:(const id [])keys count:(NSUInteger)count
{
	return [[[NSCFDictionary alloc] initWithObjects:objects forKeys:keys count:count] autorelease];
}

+ (instancetype)dictionaryWithDictionary:(NSDictionary *)dict
{
	return [[[NSCFDictionary alloc] initWithDictionary:dict] autorelease];
}

- (instancetype)init
{
	return self;
}

- (instancetype)initWithObjects:(const id [])objects forKeys:(const id [])keys count:(NSUInteger)count
{
	[self release];
	return (id)CFDictionaryCreate(kCFAllocatorDefault, (const void **)keys, (const void **)objects, (CFIndex)count, &kNSObjectDictionaryKeyCallBacks, &kNSObjectDictionaryValueCallBacks);
}

- (instancetype)initWithDictionary:(NSDictionary *)dict
{
	[self release];
	return (id)CFDictionaryCreateCopy(kCFAllocatorDefault, (CFDictionaryRef)dict);
}

- (id)copyWithZone:(NSZone *)zone
{
	(void)zone;
	return (id)CFDictionaryCreateCopy(kCFAllocatorDefault, (CFDictionaryRef)self);
}

- (id)mutableCopyWithZone:(NSZone *)zone
{
	(void)zone;
	return (id)CFDictionaryCreateMutableCopy(kCFAllocatorDefault, 0, (CFDictionaryRef)self);
}

- (id)objectForKeyedSubscript:(id)key
{
	return [self objectForKey:key];
}

@end

@implementation NSMutableDictionary

+ (instancetype)dictionaryWithCapacity:(NSUInteger)capacity
{
	return [[[NSCFDictionary alloc] initWithCapacity:capacity] autorelease];
}

- (instancetype)initWithCapacity:(NSUInteger)capacity
{
	[self release];
	return (id)CFDictionaryCreateMutable(kCFAllocatorDefault, (CFIndex)capacity, &kNSObjectDictionaryKeyCallBacks, &kNSObjectDictionaryValueCallBacks);
}

- (void)setObject:(id)anObject forKeyedSubscript:(id)key
{
	[self setObject:anObject forKey:key];
}

@end

@implementation NSCFDictionary

- (id)retain { return NSCFBridge_retain(self); }
- (oneway void)release { NSCFBridge_release(self); }
- (NSUInteger)retainCount { return NSCFBridge_retainCount(self); }
- (BOOL)isEqual:(id)other { return NSCFBridge_isEqual(self, other); }
- (NSUInteger)hash { return NSCFBridge_hash(self); }
- (NSString *)description { return NSCFBridge_description(self); }
- (void)dealloc { NSCFBridge_deallocGuard(self); }

- (NSUInteger)count
{
	return (NSUInteger)CFDictionaryGetCount((CFDictionaryRef)self);
}

- (id)objectForKey:(id)key
{
	return (id)CFDictionaryGetValue((CFDictionaryRef)self, (const void *)key);
}

- (BOOL)containsObjectForKey:(id)key
{
	return CFDictionaryContainsKey((CFDictionaryRef)self, (const void *)key) ? YES : NO;
}

- (NSArray *)allKeys
{
	CFIndex c = CFDictionaryGetCount((CFDictionaryRef)self);
	if (c == 0) {
		return [NSArray array];
	}
	const void **keys = malloc(sizeof(const void *) * (size_t)c);
	CFDictionaryGetKeysAndValues((CFDictionaryRef)self, keys, NULL);
	NSArray *result = [NSArray arrayWithObjects:(const id *)keys count:(NSUInteger)c];
	free(keys);
	return result;
}

- (NSArray *)allValues
{
	CFIndex c = CFDictionaryGetCount((CFDictionaryRef)self);
	if (c == 0) {
		return [NSArray array];
	}
	const void **values = malloc(sizeof(const void *) * (size_t)c);
	CFDictionaryGetKeysAndValues((CFDictionaryRef)self, NULL, values);
	NSArray *result = [NSArray arrayWithObjects:(const id *)values count:(NSUInteger)c];
	free(values);
	return result;
}

- (void)setObject:(id)anObject forKey:(id)key
{
	CFDictionarySetValue((CFMutableDictionaryRef)self, (const void *)key, (const void *)anObject);
}

- (void)removeObjectForKey:(id)key
{
	CFDictionaryRemoveValue((CFMutableDictionaryRef)self, (const void *)key);
}

- (void)removeAllObjects
{
	CFDictionaryRemoveAllValues((CFMutableDictionaryRef)self);
}

static void
addEntryApplier(const void *key, const void *value, void *context)
{
	CFDictionarySetValue((CFMutableDictionaryRef)context, key, value);
}

- (void)addEntriesFromDictionary:(NSDictionary *)otherDict
{
	CFDictionaryApplyFunction((CFDictionaryRef)otherDict, addEntryApplier, (void *)self);
}

@end
