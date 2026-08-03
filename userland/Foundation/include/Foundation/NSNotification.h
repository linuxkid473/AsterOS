/* Copyright (c) 2026 Vihaan Nathan
 *
 * Real NSNotificationCenter semantics: observers are NOT retained (a
 * caller must -removeObserver: before it deallocates, same real
 * footgun as actual Cocoa -- not a simplification, the genuine
 * contract). Only the classic selector-based API is implemented; the
 * modern block-based -addObserverForName:object:queue:usingBlock: is
 * not (documented v1 cut, see docs/architecture.md).
 */
#ifndef FOUNDATION_NSNOTIFICATION_H
#define FOUNDATION_NSNOTIFICATION_H

#include <Foundation/NSObject.h>

#ifdef __cplusplus
extern "C" {
#endif

@interface NSNotification : NSObject <NSCopying>

+ (instancetype)notificationWithName:(NSString *)name object:(id)object userInfo:(NSDictionary *)userInfo;

- (NSString *)name;
- (id)object;
- (NSDictionary *)userInfo;

@end

@interface NSNotificationCenter : NSObject

+ (instancetype)defaultCenter;

- (void)addObserver:(id)observer selector:(SEL)selector name:(NSString *)name object:(id)object;
- (void)removeObserver:(id)observer;
- (void)removeObserver:(id)observer name:(NSString *)name object:(id)object;

- (void)postNotification:(NSNotification *)notification;
- (void)postNotificationName:(NSString *)name object:(id)object;
- (void)postNotificationName:(NSString *)name object:(id)object userInfo:(NSDictionary *)userInfo;

@end

#ifdef __cplusplus
}
#endif

#endif /* FOUNDATION_NSNOTIFICATION_H */
