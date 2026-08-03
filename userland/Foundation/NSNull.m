/* Copyright (c) 2026 Vihaan Nathan -- see NSNull.h */
#import <Foundation/Foundation.h>
#include "NSCFBridge.h"

@interface NSCFNull : NSNull
@end

@implementation NSNull

+ (NSNull *)null
{
	return (NSNull *)kCFNull;
}

@end

@implementation NSCFNull

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
	return self;
}

@end
