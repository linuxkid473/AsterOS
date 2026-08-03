/* Copyright (c) 2026 Vihaan Nathan
 *
 * Toll-free bridged to CFURL -- filesystem URLs only, see CFURL.h's
 * header comment for the full list of what's cut (no http(s)/other
 * schemes, no relative-URL resolution, no percent-encoding).
 */
#ifndef FOUNDATION_NSURL_H
#define FOUNDATION_NSURL_H

#include <Foundation/NSObject.h>

#ifdef __cplusplus
extern "C" {
#endif

@interface NSURL : NSObject <NSCopying, NSCoding>

+ (instancetype)fileURLWithPath:(NSString *)path;
+ (instancetype)fileURLWithPath:(NSString *)path isDirectory:(BOOL)isDirectory;
+ (instancetype)URLWithString:(NSString *)URLString;

- (instancetype)initFileURLWithPath:(NSString *)path;
- (instancetype)initFileURLWithPath:(NSString *)path isDirectory:(BOOL)isDirectory;
- (instancetype)initWithString:(NSString *)URLString;

- (NSString *)path;
- (NSString *)absoluteString;
- (NSString *)lastPathComponent;
- (NSString *)pathExtension;
- (BOOL)isFileURL;
- (BOOL)hasDirectoryPath;

@end

#ifdef __cplusplus
}
#endif

#endif /* FOUNDATION_NSURL_H */
