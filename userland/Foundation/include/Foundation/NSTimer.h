/* Copyright (c) 2026 Vihaan Nathan
 *
 * Retains target/userInfo until fired-and-non-repeating or invalidated
 * -- real NSTimer's actual contract (unlike NSNotificationCenter's
 * observers, which are deliberately not retained).
 */
#ifndef FOUNDATION_NSTIMER_H
#define FOUNDATION_NSTIMER_H

#include <Foundation/NSObject.h>

#ifdef __cplusplus
extern "C" {
#endif

@class NSDate;

@interface NSTimer : NSObject

+ (instancetype)timerWithTimeInterval:(NSTimeInterval)interval target:(id)target selector:(SEL)selector userInfo:(id)userInfo repeats:(BOOL)repeats;
+ (instancetype)scheduledTimerWithTimeInterval:(NSTimeInterval)interval target:(id)target selector:(SEL)selector userInfo:(id)userInfo repeats:(BOOL)repeats;

- (void)fire;
- (void)invalidate;
- (BOOL)isValid;
- (NSDate *)fireDate;
- (NSTimeInterval)timeInterval;
- (id)userInfo;

@end

#ifdef __cplusplus
}
#endif

#endif /* FOUNDATION_NSTIMER_H */
