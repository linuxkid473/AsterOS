/* Copyright (c) 2026 Vihaan Nathan -- see NSLocale.h */
#import <Foundation/Foundation.h>
#include "NSCFBridge.h"

@interface NSCFLocale : NSLocale
@end

@implementation NSLocale

+ (id)alloc
{
	return (id)CFLocaleCopyCurrent();
}

+ (instancetype)currentLocale
{
	return [[[NSCFLocale alloc] init] autorelease];
}

+ (instancetype)systemLocale
{
	return [(id)CFRetain(CFLocaleGetSystem()) autorelease];
}

+ (instancetype)localeWithLocaleIdentifier:(NSString *)identifier
{
	return [[[NSCFLocale alloc] initWithLocaleIdentifier:identifier] autorelease];
}

- (instancetype)init
{
	return self;
}

- (instancetype)initWithLocaleIdentifier:(NSString *)identifier
{
	[self release];
	return (id)CFLocaleCreate(kCFAllocatorDefault, (CFStringRef)identifier);
}

- (id)copyWithZone:(NSZone *)zone
{
	(void)zone;
	return [self retain];
}

@end

@implementation NSCFLocale

- (id)retain { return NSCFBridge_retain(self); }
- (oneway void)release { NSCFBridge_release(self); }
- (NSUInteger)retainCount { return NSCFBridge_retainCount(self); }
- (BOOL)isEqual:(id)other { return NSCFBridge_isEqual(self, other); }
- (NSUInteger)hash { return NSCFBridge_hash(self); }
- (NSString *)description { return NSCFBridge_description(self); }
- (void)dealloc { NSCFBridge_deallocGuard(self); }

- (NSString *)localeIdentifier
{
	return (id)CFLocaleGetIdentifier((CFLocaleRef)self);
}

@end
