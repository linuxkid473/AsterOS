/* Copyright (c) 2026 Vihaan Nathan
 *
 * Toll-free bridged to CFTimeZone -- UTC-only, see CFTimeZone.h's
 * header comment for why (no tzdata in this tree's libc).
 */
#ifndef FOUNDATION_NSTIMEZONE_H
#define FOUNDATION_NSTIMEZONE_H

#include <Foundation/NSObject.h>

#ifdef __cplusplus
extern "C" {
#endif

@interface NSTimeZone : NSObject <NSCopying, NSCoding>

+ (instancetype)systemTimeZone;
+ (instancetype)defaultTimeZone;
+ (instancetype)timeZoneWithName:(NSString *)name;

- (NSString *)name;
- (NSInteger)secondsFromGMT;

@end

#ifdef __cplusplus
}
#endif

#endif /* FOUNDATION_NSTIMEZONE_H */
