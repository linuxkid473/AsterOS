/* Copyright (c) 2026 Vihaan Nathan
 *
 * Wraps this tree's real argc/argv/environ globals (userland/libc/src/
 * start.c's __libc_argc/__libc_argv, added alongside this class;
 * environ already existed).
 */
#ifndef FOUNDATION_NSPROCESSINFO_H
#define FOUNDATION_NSPROCESSINFO_H

#include <Foundation/NSObject.h>

#ifdef __cplusplus
extern "C" {
#endif

@interface NSProcessInfo : NSObject

+ (instancetype)processInfo;

- (NSString *)processName;
- (NSArray *)arguments;
- (NSDictionary *)environment;
- (NSUInteger)processIdentifier;
- (NSString *)operatingSystemVersionString;
- (NSString *)globallyUniqueString;

@end

#ifdef __cplusplus
}
#endif

#endif /* FOUNDATION_NSPROCESSINFO_H */
