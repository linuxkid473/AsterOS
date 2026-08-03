/* Copyright (c) 2026 Vihaan Nathan -- see NSData.h
 *
 * -writeToFile:atomically: v1 simplification: writes directly, no
 * temp-file-then-rename atomicity -- documented, not silent (see
 * docs/architecture.md).
 */
#import <Foundation/Foundation.h>
#include "NSCFBridge.h"
#include <stdio.h>

@interface NSCFData : NSMutableData
@end

@implementation NSData

+ (id)alloc
{
	return (id)CFDataCreateMutable(kCFAllocatorDefault, 0);
}

+ (instancetype)data
{
	return [[[NSCFData alloc] init] autorelease];
}

+ (instancetype)dataWithBytes:(const void *)bytes length:(NSUInteger)length
{
	return [[[NSCFData alloc] initWithBytes:bytes length:length] autorelease];
}

+ (instancetype)dataWithContentsOfFile:(NSString *)path
{
	return [[[NSCFData alloc] initWithContentsOfFile:path] autorelease];
}

- (instancetype)init
{
	return self;
}

- (instancetype)initWithBytes:(const void *)bytes length:(NSUInteger)length
{
	[self release];
	return (id)CFDataCreate(kCFAllocatorDefault, bytes, (CFIndex)length);
}

- (instancetype)initWithContentsOfFile:(NSString *)path
{
	FILE *f = fopen([path UTF8String], "rb");
	if (!f) {
		[self release];
		return nil;
	}
	fseek(f, 0, SEEK_END);
	long len = ftell(f);
	fseek(f, 0, SEEK_SET);
	CFMutableDataRef d = CFDataCreateMutable(kCFAllocatorDefault, (CFIndex)len);
	CFDataSetLength(d, (CFIndex)len);
	size_t n = fread(CFDataGetMutableBytePtr(d), 1, (size_t)len, f);
	fclose(f);
	CFDataSetLength(d, (CFIndex)n);
	[self release];
	return (id)d;
}

- (id)copyWithZone:(NSZone *)zone
{
	(void)zone;
	return (id)CFDataCreateCopy(kCFAllocatorDefault, (CFDataRef)self);
}

- (id)mutableCopyWithZone:(NSZone *)zone
{
	(void)zone;
	return (id)CFDataCreateMutableCopy(kCFAllocatorDefault, 0, (CFDataRef)self);
}

- (BOOL)writeToFile:(NSString *)path atomically:(BOOL)atomically
{
	(void)atomically;
	FILE *f = fopen([path UTF8String], "wb");
	if (!f) {
		return NO;
	}
	NSUInteger len = [self length];
	size_t written = len ? fwrite([self bytes], 1, len, f) : 0;
	fclose(f);
	return written == len ? YES : NO;
}

@end

@implementation NSMutableData

+ (instancetype)dataWithCapacity:(NSUInteger)capacity
{
	return [[[NSCFData alloc] initWithCapacity:capacity] autorelease];
}

- (instancetype)initWithCapacity:(NSUInteger)capacity
{
	[self release];
	return (id)CFDataCreateMutable(kCFAllocatorDefault, (CFIndex)capacity);
}

@end

@implementation NSCFData

- (id)retain { return NSCFBridge_retain(self); }
- (oneway void)release { NSCFBridge_release(self); }
- (NSUInteger)retainCount { return NSCFBridge_retainCount(self); }
- (BOOL)isEqual:(id)other { return NSCFBridge_isEqual(self, other); }
- (NSUInteger)hash { return NSCFBridge_hash(self); }
- (NSString *)description { return NSCFBridge_description(self); }
- (void)dealloc { NSCFBridge_deallocGuard(self); }

- (NSUInteger)length
{
	return (NSUInteger)CFDataGetLength((CFDataRef)self);
}

- (const void *)bytes
{
	return CFDataGetBytePtr((CFDataRef)self);
}

- (void)getBytes:(void *)buffer length:(NSUInteger)length
{
	CFDataGetBytes((CFDataRef)self, CFRangeMake(0, (CFIndex)length), buffer);
}

- (BOOL)isEqualToData:(NSData *)other
{
	if (!other) {
		return NO;
	}
	return CFEqual((CFTypeRef)self, (CFTypeRef)other) ? YES : NO;
}

- (void *)mutableBytes
{
	return CFDataGetMutableBytePtr((CFMutableDataRef)self);
}

- (void)appendBytes:(const void *)bytes length:(NSUInteger)length
{
	CFDataAppendBytes((CFMutableDataRef)self, bytes, (CFIndex)length);
}

- (void)appendData:(NSData *)other
{
	CFDataAppendBytes((CFMutableDataRef)self, [other bytes], (CFIndex)[other length]);
}

- (void)setLength:(NSUInteger)length
{
	CFDataSetLength((CFMutableDataRef)self, (CFIndex)length);
}

@end
