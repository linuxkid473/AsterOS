/* Copyright (c) 2026 Vihaan Nathan
 *
 * v1 scope cut, documented: no real .bundle/.app package structure (no
 * Info.plist, no Resources/ subdirectory, no per-platform resource
 * variants) -- this OS has no such packaging convention yet. A
 * "bundle" here is just a directory; -resourcePath equals -bundlePath
 * and -pathForResource:ofType: looks directly inside it. +mainBundle's
 * path is the directory containing the running executable (via
 * NSProcessInfo/argv[0]).
 */
#ifndef FOUNDATION_NSBUNDLE_H
#define FOUNDATION_NSBUNDLE_H

#include <Foundation/NSObject.h>

#ifdef __cplusplus
extern "C" {
#endif

@interface NSBundle : NSObject

+ (instancetype)mainBundle;
+ (instancetype)bundleWithPath:(NSString *)path;

- (instancetype)initWithPath:(NSString *)path;

- (NSString *)bundlePath;
- (NSString *)resourcePath;
- (NSString *)pathForResource:(NSString *)name ofType:(NSString *)ext;

@end

#ifdef __cplusplus
}
#endif

#endif /* FOUNDATION_NSBUNDLE_H */
