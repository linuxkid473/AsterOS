/* Copyright (c) 2026 Vihaan Nathan -- see NSURL.h */
#import <Foundation/Foundation.h>
#include "NSCFBridge.h"

@interface NSCFURL : NSURL
@end

@implementation NSURL

+ (id)alloc
{
	CFStringRef empty = CFStringCreateWithCString(kCFAllocatorDefault, "", kCFStringEncodingUTF8);
	CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, empty, kCFURLPOSIXPathStyle, false);
	CFRelease(empty);
	return (id)url;
}

+ (instancetype)fileURLWithPath:(NSString *)path
{
	return [[[NSCFURL alloc] initFileURLWithPath:path] autorelease];
}

+ (instancetype)fileURLWithPath:(NSString *)path isDirectory:(BOOL)isDirectory
{
	return [[[NSCFURL alloc] initFileURLWithPath:path isDirectory:isDirectory] autorelease];
}

+ (instancetype)URLWithString:(NSString *)URLString
{
	return [[[NSCFURL alloc] initWithString:URLString] autorelease];
}

- (instancetype)initFileURLWithPath:(NSString *)path
{
	return [self initFileURLWithPath:path isDirectory:NO];
}

- (instancetype)initFileURLWithPath:(NSString *)path isDirectory:(BOOL)isDirectory
{
	[self release];
	return (id)CFURLCreateWithFileSystemPath(kCFAllocatorDefault, (CFStringRef)path, kCFURLPOSIXPathStyle, isDirectory);
}

- (instancetype)initWithString:(NSString *)URLString
{
	[self release];
	return (id)CFURLCreateWithString(kCFAllocatorDefault, (CFStringRef)URLString, NULL);
}

- (id)copyWithZone:(NSZone *)zone
{
	(void)zone;
	return [self retain];
}

@end

@implementation NSCFURL

- (id)retain { return NSCFBridge_retain(self); }
- (oneway void)release { NSCFBridge_release(self); }
- (NSUInteger)retainCount { return NSCFBridge_retainCount(self); }
- (BOOL)isEqual:(id)other { return NSCFBridge_isEqual(self, other); }
- (NSUInteger)hash { return NSCFBridge_hash(self); }
- (NSString *)description { return NSCFBridge_description(self); }
- (void)dealloc { NSCFBridge_deallocGuard(self); }

- (NSString *)path
{
	return [(id)CFURLCopyFileSystemPath((CFURLRef)self, kCFURLPOSIXPathStyle) autorelease];
}

- (NSString *)absoluteString
{
	return [(id)CFURLGetString((CFURLRef)self) autorelease];
}

- (NSString *)lastPathComponent
{
	return [(id)CFURLCopyLastPathComponent((CFURLRef)self) autorelease];
}

- (NSString *)pathExtension
{
	return [(id)CFURLCopyPathExtension((CFURLRef)self) autorelease];
}

- (BOOL)isFileURL
{
	return YES;	/* the only scheme this tree supports -- see CFURL.h */
}

- (BOOL)hasDirectoryPath
{
	return CFURLHasDirectoryPath((CFURLRef)self) ? YES : NO;
}

@end
