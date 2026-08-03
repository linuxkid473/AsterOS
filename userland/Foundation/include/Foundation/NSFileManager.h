/* Copyright (c) 2026 Vihaan Nathan
 *
 * A thin wrapper over this tree's real syscalls (userland/libc/src/
 * syscalls.c/dirent.c) -- not CF-bridged (real Darwin's NSFileManager
 * isn't either). v1 cuts, documented: no -copyItemAtPath:/
 * -moveItemAtPath: (implement via -contentsAtPath:/-createFileAtPath:
 * and two calls if needed), no file attributes/permissions beyond the
 * default mode passed to -createDirectoryAtPath:, no symlink-specific
 * API.
 */
#ifndef FOUNDATION_NSFILEMANAGER_H
#define FOUNDATION_NSFILEMANAGER_H

#include <Foundation/NSObject.h>

#ifdef __cplusplus
extern "C" {
#endif

@class NSError;
@class NSData;

@interface NSFileManager : NSObject

+ (instancetype)defaultManager;

- (BOOL)fileExistsAtPath:(NSString *)path;
- (BOOL)fileExistsAtPath:(NSString *)path isDirectory:(BOOL *)isDirectory;

- (BOOL)createDirectoryAtPath:(NSString *)path withIntermediateDirectories:(BOOL)createIntermediates attributes:(NSDictionary *)attributes error:(NSError **)error;
- (BOOL)removeItemAtPath:(NSString *)path error:(NSError **)error;

- (NSArray *)contentsOfDirectoryAtPath:(NSString *)path error:(NSError **)error;

- (NSString *)currentDirectoryPath;
- (BOOL)changeCurrentDirectoryPath:(NSString *)path;

- (NSData *)contentsAtPath:(NSString *)path;
- (BOOL)createFileAtPath:(NSString *)path contents:(NSData *)data attributes:(NSDictionary *)attributes;

@end

#ifdef __cplusplus
}
#endif

#endif /* FOUNDATION_NSFILEMANAGER_H */
