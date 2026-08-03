/* Copyright (c) 2026 Vihaan Nathan
 *
 * Minimal, v1: a single global run loop (+currentRunLoop and
 * +mainRunLoop return the same shared instance) driving a sorted
 * NSTimer list only -- no fd/port-based input sources (this tree's
 * libc does have a real poll(2), see userland/libc/src/syscalls.c, but
 * wiring CFRunLoopSource-style fd registration through to it is out of
 * scope for this pass), no run loop modes beyond a single default one,
 * no nested run loops. Documented, not silent -- see
 * docs/architecture.md. Sleeps via this tree's real nanosleep
 * (userland/libc), not a busy-poll.
 */
#ifndef FOUNDATION_NSRUNLOOP_H
#define FOUNDATION_NSRUNLOOP_H

#include <Foundation/NSObject.h>

#ifdef __cplusplus
extern "C" {
#endif

@class NSTimer;
@class NSDate;

/* Not `const` -- see NSException.h's header comment on this exact
 * pattern. Read-only in practice. */
FOUNDATION_EXPORT NSString *NSDefaultRunLoopMode;

@interface NSRunLoop : NSObject

+ (instancetype)currentRunLoop;
+ (instancetype)mainRunLoop;

- (void)addTimer:(NSTimer *)timer forMode:(NSString *)mode;
- (void)runUntilDate:(NSDate *)limitDate;
- (BOOL)runMode:(NSString *)mode beforeDate:(NSDate *)limitDate;

@end

#ifdef __cplusplus
}
#endif

#endif /* FOUNDATION_NSRUNLOOP_H */
