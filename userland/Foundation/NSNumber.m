/* Copyright (c) 2026 Vihaan Nathan
 *
 * Two concrete backing classes, matching real Darwin's own split:
 * NSCFNumber (int64_t/double via CFNumber) and NSCFBoolean (CFBoolean).
 * +numberWithBool: returns the shared kCFBooleanTrue/kCFBooleanFalse
 * singletons rather than allocating, same as real Foundation.
 */
#import <Foundation/Foundation.h>
#include "NSCFBridge.h"
#include <stdio.h>

@interface NSCFNumber : NSNumber
@end

@interface NSCFBoolean : NSNumber
@end

@implementation NSNumber

+ (id)alloc
{
	int64_t zero = 0;
	return (id)CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt64Type, &zero);
}

+ (instancetype)numberWithInt:(int)value
{
	return [[[NSCFNumber alloc] initWithInt:value] autorelease];
}

+ (instancetype)numberWithUnsignedInt:(unsigned int)value
{
	return [self numberWithLongLong:(long long)value];
}

+ (instancetype)numberWithInteger:(NSInteger)value
{
	return [self numberWithLongLong:(long long)value];
}

+ (instancetype)numberWithLong:(long)value
{
	return [self numberWithLongLong:(long long)value];
}

+ (instancetype)numberWithLongLong:(long long)value
{
	int64_t v = value;
	return [(id)CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt64Type, &v) autorelease];
}

+ (instancetype)numberWithDouble:(double)value
{
	return [[[NSCFNumber alloc] initWithDouble:value] autorelease];
}

+ (instancetype)numberWithFloat:(float)value
{
	return [self numberWithDouble:(double)value];
}

+ (instancetype)numberWithChar:(char)value
{
	return [self numberWithInt:(int)value];
}

+ (instancetype)numberWithBool:(BOOL)value
{
	/* Shared singletons, not a fresh allocation -- kCFBooleanTrue/False
	 * are already isa-tagged NSCFBoolean by FoundationInit.m. retain
	 * (then autorelease, matching every other convenience constructor's
	 * ownership convention) is safe to call even though CFRetain/
	 * CFRelease on a constant CF singleton are documented no-ops (see
	 * CFRuntime.c) -- retainCount just never moves. */
	id num = (id)(value ? kCFBooleanTrue : kCFBooleanFalse);
	return [[num retain] autorelease];
}

- (instancetype)initWithInt:(int)value
{
	[self release];
	int64_t v = value;
	return (id)CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt64Type, &v);
}

- (instancetype)initWithInteger:(NSInteger)value
{
	return [self initWithInt:(int)value];
}

- (instancetype)initWithDouble:(double)value
{
	[self release];
	return (id)CFNumberCreate(kCFAllocatorDefault, kCFNumberDoubleType, &value);
}

- (instancetype)initWithBool:(BOOL)value
{
	[self release];
	return [(id)(value ? kCFBooleanTrue : kCFBooleanFalse) retain];
}

- (NSString *)stringValue
{
	return [self description];
}

- (BOOL)isEqualToNumber:(NSNumber *)other
{
	if (!other) {
		return NO;
	}
	return [self doubleValue] == [other doubleValue];
}

- (NSComparisonResult)compare:(NSNumber *)other
{
	double a = [self doubleValue], b = [other doubleValue];
	if (a < b) {
		return NSOrderedAscending;
	}
	if (a > b) {
		return NSOrderedDescending;
	}
	return NSOrderedSame;
}

@end

@implementation NSCFNumber

- (id)retain { return NSCFBridge_retain(self); }
- (oneway void)release { NSCFBridge_release(self); }
- (NSUInteger)retainCount { return NSCFBridge_retainCount(self); }
- (BOOL)isEqual:(id)other { return NSCFBridge_isEqual(self, other); }
- (NSUInteger)hash { return NSCFBridge_hash(self); }
- (NSString *)description { return NSCFBridge_description(self); }
- (void)dealloc { NSCFBridge_deallocGuard(self); }

- (id)copyWithZone:(NSZone *)zone
{
	(void)zone;
	return [self retain];	/* immutable value type: sharing is safe, matches real NSNumber */
}

- (int)intValue { double d; CFNumberGetValue((CFNumberRef)self, kCFNumberDoubleType, &d); return (int)d; }
- (unsigned int)unsignedIntValue { return (unsigned int)[self intValue]; }
- (NSInteger)integerValue { return (NSInteger)[self longLongValue]; }
- (long)longValue { return (long)[self longLongValue]; }
- (long long)longLongValue { int64_t v; CFNumberGetValue((CFNumberRef)self, kCFNumberSInt64Type, &v); return v; }
- (double)doubleValue { double v; CFNumberGetValue((CFNumberRef)self, kCFNumberDoubleType, &v); return v; }
- (float)floatValue { return (float)[self doubleValue]; }
- (char)charValue { return (char)[self intValue]; }
- (BOOL)boolValue { return [self doubleValue] != 0.0; }

@end

@implementation NSCFBoolean

- (id)retain { return NSCFBridge_retain(self); }
- (oneway void)release { NSCFBridge_release(self); }
- (NSUInteger)retainCount { return NSCFBridge_retainCount(self); }
- (BOOL)isEqual:(id)other { return NSCFBridge_isEqual(self, other); }
- (NSUInteger)hash { return NSCFBridge_hash(self); }
- (NSString *)description { return NSCFBridge_description(self); }
- (void)dealloc { NSCFBridge_deallocGuard(self); }

- (id)copyWithZone:(NSZone *)zone
{
	(void)zone;
	return [self retain];
}

- (BOOL)boolValue { return CFBooleanGetValue((CFBooleanRef)self) ? YES : NO; }
- (int)intValue { return [self boolValue] ? 1 : 0; }
- (unsigned int)unsignedIntValue { return [self boolValue] ? 1 : 0; }
- (NSInteger)integerValue { return [self boolValue] ? 1 : 0; }
- (long)longValue { return [self boolValue] ? 1 : 0; }
- (long long)longLongValue { return [self boolValue] ? 1 : 0; }
- (double)doubleValue { return [self boolValue] ? 1.0 : 0.0; }
- (float)floatValue { return [self boolValue] ? 1.0f : 0.0f; }
- (char)charValue { return [self boolValue] ? 1 : 0; }

@end
