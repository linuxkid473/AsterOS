/* Copyright (c) 2026 Vihaan Nathan -- see NSProcessInfo.h */
#import <Foundation/Foundation.h>
#include <unistd.h>
#include <string.h>

extern int __libc_argc;
extern char **__libc_argv;
extern char **environ;

@implementation NSProcessInfo

+ (instancetype)processInfo
{
	static NSProcessInfo *shared;
	if (!shared) {
		shared = [[self alloc] init];
	}
	return shared;
}

- (NSString *)processName
{
	const char *argv0 = (__libc_argc > 0 && __libc_argv) ? __libc_argv[0] : "";
	const char *slash = strrchr(argv0, '/');
	return [NSString stringWithUTF8String:slash ? slash + 1 : argv0];
}

- (NSArray *)arguments
{
	if (!__libc_argv || __libc_argc <= 0) {
		return [NSArray array];
	}
	NSMutableArray *result = [NSMutableArray arrayWithCapacity:(NSUInteger)__libc_argc];
	for (int i = 0; i < __libc_argc; i++) {
		[result addObject:[NSString stringWithUTF8String:__libc_argv[i]]];
	}
	return result;
}

- (NSDictionary *)environment
{
	NSMutableDictionary *result = [NSMutableDictionary dictionaryWithCapacity:0];
	for (char **e = environ; e && *e; e++) {
		const char *eq = strchr(*e, '=');
		if (!eq) {
			continue;
		}
		NSString *key = [[[NSString alloc] initWithBytes:*e length:(NSUInteger)(eq - *e) encoding:NSUTF8StringEncoding] autorelease];
		NSString *value = [NSString stringWithUTF8String:eq + 1];
		[result setObject:value forKey:key];
	}
	return result;
}

- (NSUInteger)processIdentifier
{
	return (NSUInteger)getpid();
}

- (NSString *)operatingSystemVersionString
{
	return [NSString stringWithUTF8String:"AsterOS (Darwin-compatible), development build"];
}

- (NSString *)globallyUniqueString
{
	static unsigned long counter;
	return [NSString stringWithFormat:[NSString stringWithUTF8String:"%lu-%lu-%lu"], (unsigned long)getpid(), (unsigned long)CFAbsoluteTimeGetCurrent(), counter++];
}

@end
