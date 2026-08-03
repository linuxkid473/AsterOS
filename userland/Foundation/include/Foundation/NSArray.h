/* Copyright (c) 2026 Vihaan Nathan
 *
 * Toll-free bridged to CFArray (userland/CoreFoundation/CFArray.h).
 * v1 scope cut, documented not silent: no NSFastEnumeration
 * (-countByEnumeratingWithState:objects:count:, the `for (id x in
 * array)` protocol) and no NSEnumerator -- indexed access
 * (-objectAtIndex:/-count) covers every real use in this tree today.
 * Inherits CFArray's O(n) CFArrayGetFirstIndexOfValue for
 * -indexOfObject:/-containsObject:.
 */
#ifndef FOUNDATION_NSARRAY_H
#define FOUNDATION_NSARRAY_H

#include <Foundation/NSObject.h>

#ifdef __cplusplus
extern "C" {
#endif

@interface NSArray : NSObject <NSCopying, NSMutableCopying, NSCoding>

+ (instancetype)array;
+ (instancetype)arrayWithObject:(id)anObject;
+ (instancetype)arrayWithObjects:(const id [])objects count:(NSUInteger)count;
+ (instancetype)arrayWithArray:(NSArray *)array;

- (instancetype)initWithObjects:(const id [])objects count:(NSUInteger)count;
- (instancetype)initWithArray:(NSArray *)array;

- (NSUInteger)count;
- (id)objectAtIndex:(NSUInteger)index;
- (id)firstObject;
- (id)lastObject;
- (BOOL)containsObject:(id)anObject;
- (NSUInteger)indexOfObject:(id)anObject;
- (id)objectAtIndexedSubscript:(NSUInteger)idx;

- (NSArray *)arrayByAddingObject:(id)anObject;
- (NSString *)componentsJoinedByString:(NSString *)separator;

@end

@interface NSMutableArray : NSArray

+ (instancetype)arrayWithCapacity:(NSUInteger)capacity;
- (instancetype)initWithCapacity:(NSUInteger)capacity;

- (void)addObject:(id)anObject;
- (void)insertObject:(id)anObject atIndex:(NSUInteger)index;
- (void)removeObjectAtIndex:(NSUInteger)index;
- (void)removeObject:(id)anObject;
- (void)removeAllObjects;
- (void)replaceObjectAtIndex:(NSUInteger)index withObject:(id)anObject;
- (void)addObjectsFromArray:(NSArray *)otherArray;

@end

#ifdef __cplusplus
}
#endif

#endif /* FOUNDATION_NSARRAY_H */
