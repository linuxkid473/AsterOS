/* Copyright (c) 2026 Vihaan Nathan
 *
 * Toll-free bridged to CFDate (userland/CoreFoundation/CFDate.h).
 */
#ifndef FOUNDATION_NSDATE_H
#define FOUNDATION_NSDATE_H

#include <Foundation/NSObject.h>

#ifdef __cplusplus
extern "C" {
#endif

@interface NSDate : NSObject <NSCopying, NSCoding>

+ (instancetype)date;
+ (instancetype)dateWithTimeIntervalSinceNow:(NSTimeInterval)seconds;
+ (instancetype)dateWithTimeIntervalSince1970:(NSTimeInterval)seconds;
+ (instancetype)distantPast;
+ (instancetype)distantFuture;

- (instancetype)initWithTimeIntervalSinceNow:(NSTimeInterval)seconds;
- (instancetype)initWithTimeIntervalSince1970:(NSTimeInterval)seconds;

- (NSTimeInterval)timeIntervalSince1970;
- (NSTimeInterval)timeIntervalSinceDate:(NSDate *)other;
- (NSTimeInterval)timeIntervalSinceNow;
- (NSComparisonResult)compare:(NSDate *)other;
- (BOOL)isEqualToDate:(NSDate *)other;
- (NSDate *)earlierDate:(NSDate *)other;
- (NSDate *)laterDate:(NSDate *)other;

@end

#ifdef __cplusplus
}
#endif

#endif /* FOUNDATION_NSDATE_H */
