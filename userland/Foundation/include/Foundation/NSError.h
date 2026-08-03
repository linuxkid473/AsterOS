/* Copyright (c) 2026 Vihaan Nathan
 *
 * Plain ivar-backed NSObject subclass, not CF-bridged -- this tree's
 * CoreFoundation has no CFError type to bridge to (documented, see
 * docs/architecture.md; adding one would be pure overhead, nothing in
 * this tree round-trips an NSError through CF API).
 */
#ifndef FOUNDATION_NSERROR_H
#define FOUNDATION_NSERROR_H

#include <Foundation/NSObject.h>

#ifdef __cplusplus
extern "C" {
#endif

@class NSDictionary;

/* Not `const` -- see NSException.h's header comment on this exact
 * pattern (real object, constructor-initialized, no `@"literal"`
 * support in this tree). Read-only in practice. */
FOUNDATION_EXPORT NSString *NSCocoaErrorDomain;
FOUNDATION_EXPORT NSString *NSPOSIXErrorDomain;
FOUNDATION_EXPORT NSString *NSLocalizedDescriptionKey;

@interface NSError : NSObject <NSCopying, NSCoding>

+ (instancetype)errorWithDomain:(NSString *)domain code:(NSInteger)code userInfo:(NSDictionary *)dict;
- (instancetype)initWithDomain:(NSString *)domain code:(NSInteger)code userInfo:(NSDictionary *)dict;

- (NSString *)domain;
- (NSInteger)code;
- (NSDictionary *)userInfo;
- (NSString *)localizedDescription;

@end

#ifdef __cplusplus
}
#endif

#endif /* FOUNDATION_NSERROR_H */
