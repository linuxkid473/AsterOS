/* Copyright (c) 2026 Vihaan Nathan -- see NSDate.h */
#import <Foundation/Foundation.h>
#include "NSCFBridge.h"

@interface NSCFDate : NSDate
@end

@implementation NSDate

+ (id)alloc
{
	return (id)CFDateCreate(kCFAllocatorDefault, 0);
}

+ (instancetype)date
{
	return [[[NSCFDate alloc] init] autorelease];
}

+ (instancetype)dateWithTimeIntervalSinceNow:(NSTimeInterval)seconds
{
	return [[[NSCFDate alloc] initWithTimeIntervalSinceNow:seconds] autorelease];
}

+ (instancetype)dateWithTimeIntervalSince1970:(NSTimeInterval)seconds
{
	return [[[NSCFDate alloc] initWithTimeIntervalSince1970:seconds] autorelease];
}

+ (instancetype)distantPast
{
	return [[[NSCFDate alloc] initWithTimeIntervalSince1970:-63113904000.0] autorelease];	/* year 0001, matches real Foundation's magnitude */
}

+ (instancetype)distantFuture
{
	return [[[NSCFDate alloc] initWithTimeIntervalSince1970:64092211200.0] autorelease];	/* year 4001 */
}

- (instancetype)init
{
	[self release];
	return (id)CFDateCreate(kCFAllocatorDefault, CFAbsoluteTimeGetCurrent());
}

- (instancetype)initWithTimeIntervalSinceNow:(NSTimeInterval)seconds
{
	[self release];
	return (id)CFDateCreate(kCFAllocatorDefault, CFAbsoluteTimeGetCurrent() + seconds);
}

- (instancetype)initWithTimeIntervalSince1970:(NSTimeInterval)seconds
{
	[self release];
	return (id)CFDateCreate(kCFAllocatorDefault, seconds - kCFAbsoluteTimeIntervalSince1970);
}

- (id)copyWithZone:(NSZone *)zone
{
	(void)zone;
	return [self retain];	/* immutable value type: sharing is safe */
}

@end

@implementation NSCFDate

- (id)retain { return NSCFBridge_retain(self); }
- (oneway void)release { NSCFBridge_release(self); }
- (NSUInteger)retainCount { return NSCFBridge_retainCount(self); }
- (BOOL)isEqual:(id)other { return NSCFBridge_isEqual(self, other); }
- (NSUInteger)hash { return NSCFBridge_hash(self); }
- (NSString *)description { return NSCFBridge_description(self); }
- (void)dealloc { NSCFBridge_deallocGuard(self); }

- (NSTimeInterval)timeIntervalSince1970
{
	return CFDateGetAbsoluteTime((CFDateRef)self) + kCFAbsoluteTimeIntervalSince1970;
}

- (NSTimeInterval)timeIntervalSinceDate:(NSDate *)other
{
	return CFDateGetTimeIntervalSinceDate((CFDateRef)self, (CFDateRef)other);
}

- (NSTimeInterval)timeIntervalSinceNow
{
	return CFDateGetAbsoluteTime((CFDateRef)self) - CFAbsoluteTimeGetCurrent();
}

- (NSComparisonResult)compare:(NSDate *)other
{
	return (NSComparisonResult)CFDateCompare((CFDateRef)self, (CFDateRef)other, NULL);
}

- (BOOL)isEqualToDate:(NSDate *)other
{
	if (!other) {
		return NO;
	}
	return CFEqual((CFTypeRef)self, (CFTypeRef)other) ? YES : NO;
}

- (NSDate *)earlierDate:(NSDate *)other
{
	return [self compare:other] == NSOrderedDescending ? other : self;
}

- (NSDate *)laterDate:(NSDate *)other
{
	return [self compare:other] == NSOrderedAscending ? other : self;
}

@end
