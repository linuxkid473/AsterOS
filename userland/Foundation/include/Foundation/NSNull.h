/* Copyright (c) 2026 Vihaan Nathan
 *
 * Toll-free bridged to CFNull's single static singleton kCFNull
 * (userland/CoreFoundation/CFNull.c) -- +null always returns that exact
 * object, isa-tagged NSCFNull by FoundationInit.m.
 */
#ifndef FOUNDATION_NSNULL_H
#define FOUNDATION_NSNULL_H

#include <Foundation/NSObject.h>

#ifdef __cplusplus
extern "C" {
#endif

@interface NSNull : NSObject <NSCopying, NSCoding>

+ (NSNull *)null;

@end

#ifdef __cplusplus
}
#endif

#endif /* FOUNDATION_NSNULL_H */
