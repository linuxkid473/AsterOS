/* Copyright (c) 2026 Vihaan Nathan
 *
 * Toll-free bridged to CFSet (userland/CoreFoundation/CFSet.h) --
 * inherits its O(n) linear-array lookup. No NSFastEnumeration, same v1
 * cut as NSArray.h/NSDictionary.h -- use -allObjects for iteration.
 */
#ifndef FOUNDATION_NSSET_H
#define FOUNDATION_NSSET_H

#include <Foundation/NSObject.h>

#ifdef __cplusplus
extern "C" {
#endif

@interface NSSet : NSObject <NSCopying, NSMutableCopying, NSCoding>

+ (instancetype)set;
+ (instancetype)setWithObject:(id)anObject;
+ (instancetype)setWithObjects:(const id [])objects count:(NSUInteger)count;
+ (instancetype)setWithSet:(NSSet *)set;

- (instancetype)initWithObjects:(const id [])objects count:(NSUInteger)count;
- (instancetype)initWithSet:(NSSet *)set;

- (NSUInteger)count;
- (BOOL)containsObject:(id)anObject;
- (id)anyObject;
- (NSArray *)allObjects;

@end

@interface NSMutableSet : NSSet

+ (instancetype)setWithCapacity:(NSUInteger)capacity;
- (instancetype)initWithCapacity:(NSUInteger)capacity;

- (void)addObject:(id)anObject;
- (void)removeObject:(id)anObject;
- (void)removeAllObjects;

@end

#ifdef __cplusplus
}
#endif

#endif /* FOUNDATION_NSSET_H */
