/* Copyright (c) 2026 Vihaan Nathan -- see NSTimeZone.h */
#import <Foundation/Foundation.h>
#include "NSCFBridge.h"

@interface NSCFTimeZone : NSTimeZone
@end

@implementation NSTimeZone

+ (id)alloc
{
	return (id)CFTimeZoneCopySystem();
}

+ (instancetype)systemTimeZone
{
	return [(id)CFTimeZoneCopySystem() autorelease];
}

+ (instancetype)defaultTimeZone
{
	return [(id)CFTimeZoneCopyDefault() autorelease];
}

+ (instancetype)timeZoneWithName:(NSString *)name
{
	CFTimeZoneRef tz = CFTimeZoneCreateWithName(kCFAllocatorDefault, (CFStringRef)name, false);
	return tz ? [(id)tz autorelease] : nil;
}

- (id)copyWithZone:(NSZone *)zone
{
	(void)zone;
	return [self retain];
}

@end

@implementation NSCFTimeZone

- (id)retain { return NSCFBridge_retain(self); }
- (oneway void)release { NSCFBridge_release(self); }
- (NSUInteger)retainCount { return NSCFBridge_retainCount(self); }
- (BOOL)isEqual:(id)other { return NSCFBridge_isEqual(self, other); }
- (NSUInteger)hash { return NSCFBridge_hash(self); }
- (NSString *)description { return NSCFBridge_description(self); }
- (void)dealloc { NSCFBridge_deallocGuard(self); }

- (NSString *)name
{
	return (id)CFTimeZoneGetName((CFTimeZoneRef)self);
}

- (NSInteger)secondsFromGMT
{
	return (NSInteger)CFTimeZoneGetSecondsFromGMT((CFTimeZoneRef)self, CFAbsoluteTimeGetCurrent());
}

@end
