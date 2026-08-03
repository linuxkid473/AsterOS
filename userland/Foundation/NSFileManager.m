/* Copyright (c) 2026 Vihaan Nathan -- see NSFileManager.h */
#import <Foundation/Foundation.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

static NSError *
posixError(void)
{
	return [NSError errorWithDomain:NSPOSIXErrorDomain code:errno userInfo:nil];
}

@implementation NSFileManager

+ (instancetype)defaultManager
{
	static NSFileManager *shared;
	if (!shared) {
		shared = [[self alloc] init];
	}
	return shared;
}

- (BOOL)fileExistsAtPath:(NSString *)path
{
	return [self fileExistsAtPath:path isDirectory:NULL];
}

- (BOOL)fileExistsAtPath:(NSString *)path isDirectory:(BOOL *)isDirectory
{
	struct stat sb;
	if (stat([path UTF8String], &sb) != 0) {
		return NO;
	}
	if (isDirectory) {
		*isDirectory = S_ISDIR(sb.st_mode) ? YES : NO;
	}
	return YES;
}

- (BOOL)createDirectoryAtPath:(NSString *)path withIntermediateDirectories:(BOOL)createIntermediates attributes:(NSDictionary *)attributes error:(NSError **)error
{
	(void)attributes;	/* v1: default mode only, no attribute dictionary support -- see header comment */
	const char *cpath = [path UTF8String];
	if (!createIntermediates) {
		if (mkdir(cpath, 0755) != 0 && errno != EEXIST) {
			if (error) {
				*error = posixError();
			}
			return NO;
		}
		return YES;
	}

	char buf[1024];
	size_t len = strlen(cpath);
	if (len >= sizeof(buf)) {
		if (error) {
			*error = [NSError errorWithDomain:NSPOSIXErrorDomain code:ENAMETOOLONG userInfo:nil];
		}
		return NO;
	}
	strcpy(buf, cpath);
	for (size_t i = 1; i < len; i++) {
		if (buf[i] != '/') {
			continue;
		}
		buf[i] = '\0';
		if (mkdir(buf, 0755) != 0 && errno != EEXIST) {
			if (error) {
				*error = posixError();
			}
			return NO;
		}
		buf[i] = '/';
	}
	if (mkdir(buf, 0755) != 0 && errno != EEXIST) {
		if (error) {
			*error = posixError();
		}
		return NO;
	}
	return YES;
}

- (BOOL)removeItemAtPath:(NSString *)path error:(NSError **)error
{
	const char *cpath = [path UTF8String];
	BOOL isDir = NO;
	if (![self fileExistsAtPath:path isDirectory:&isDir]) {
		if (error) {
			*error = [NSError errorWithDomain:NSPOSIXErrorDomain code:ENOENT userInfo:nil];
		}
		return NO;
	}
	int rc = isDir ? rmdir(cpath) : unlink(cpath);
	if (rc != 0) {
		if (error) {
			*error = posixError();
		}
		return NO;
	}
	return YES;
}

- (NSArray *)contentsOfDirectoryAtPath:(NSString *)path error:(NSError **)error
{
	DIR *d = opendir([path UTF8String]);
	if (!d) {
		if (error) {
			*error = posixError();
		}
		return nil;
	}
	NSMutableArray *result = [NSMutableArray arrayWithCapacity:0];
	struct dirent *ent;
	while ((ent = readdir(d)) != NULL) {
		if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
			continue;
		}
		[result addObject:[NSString stringWithUTF8String:ent->d_name]];
	}
	closedir(d);
	return result;
}

- (NSString *)currentDirectoryPath
{
	char buf[1024];
	if (!getcwd(buf, sizeof(buf))) {
		return nil;
	}
	return [NSString stringWithUTF8String:buf];
}

- (BOOL)changeCurrentDirectoryPath:(NSString *)path
{
	return chdir([path UTF8String]) == 0 ? YES : NO;
}

- (NSData *)contentsAtPath:(NSString *)path
{
	return [NSData dataWithContentsOfFile:path];
}

- (BOOL)createFileAtPath:(NSString *)path contents:(NSData *)data attributes:(NSDictionary *)attributes
{
	(void)attributes;
	if (!data) {
		data = [NSData data];
	}
	return [data writeToFile:path atomically:NO];
}

@end
