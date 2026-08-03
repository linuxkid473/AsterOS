/* Copyright (c) 2026 Vihaan Nathan -- see NSBundle.h */
#import <Foundation/Foundation.h>
#include <string.h>

@implementation NSBundle
{
	NSString *_path;
}

+ (instancetype)mainBundle
{
	static NSBundle *shared;
	if (shared) {
		return shared;
	}
	const char *argv0 = [[[NSProcessInfo processInfo] arguments] count] > 0
		? [[[[NSProcessInfo processInfo] arguments] objectAtIndex:0] UTF8String]
		: "";
	const char *slash = strrchr(argv0, '/');
	NSString *dir;
	if (slash) {
		dir = [[[NSString alloc] initWithBytes:argv0 length:(NSUInteger)(slash - argv0) encoding:NSUTF8StringEncoding] autorelease];
	} else {
		dir = [[NSFileManager defaultManager] currentDirectoryPath];
	}
	shared = [[self alloc] initWithPath:dir];
	return shared;
}

+ (instancetype)bundleWithPath:(NSString *)path
{
	return [[[self alloc] initWithPath:path] autorelease];
}

- (instancetype)initWithPath:(NSString *)path
{
	self = [super init];
	if (self) {
		_path = [path retain];
	}
	return self;
}

- (void)dealloc
{
	[_path release];
	[super dealloc];
}

- (NSString *)bundlePath
{
	return _path;
}

- (NSString *)resourcePath
{
	return _path;	/* no Resources/ subdirectory -- see header comment */
}

- (NSString *)pathForResource:(NSString *)name ofType:(NSString *)ext
{
	NSString *fileName = (ext && [ext length] > 0)
		? [NSString stringWithFormat:[NSString stringWithUTF8String:"%@.%@"], name, ext]
		: name;
	NSString *full = [_path stringByAppendingFormat:[NSString stringWithUTF8String:"/%@"], fileName];
	return [[NSFileManager defaultManager] fileExistsAtPath:full] ? full : nil;
}

@end
