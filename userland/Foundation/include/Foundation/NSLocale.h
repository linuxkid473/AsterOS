/* Copyright (c) 2026 Vihaan Nathan
 *
 * Toll-free bridged to CFLocale -- "en_US_POSIX"-only, see CFLocale.h's
 * header comment for why (no real locale data in this tree's libc).
 */
#ifndef FOUNDATION_NSLOCALE_H
#define FOUNDATION_NSLOCALE_H

#include <Foundation/NSObject.h>

#ifdef __cplusplus
extern "C" {
#endif

@interface NSLocale : NSObject <NSCopying, NSCoding>

+ (instancetype)currentLocale;
+ (instancetype)systemLocale;
+ (instancetype)localeWithLocaleIdentifier:(NSString *)identifier;

- (instancetype)initWithLocaleIdentifier:(NSString *)identifier;
- (NSString *)localeIdentifier;

@end

#ifdef __cplusplus
}
#endif

#endif /* FOUNDATION_NSLOCALE_H */
