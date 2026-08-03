/* Copyright (c) 2026 Vihaan Nathan
 *
 * Toll-free bridged to CFDictionary (userland/CoreFoundation/
 * CFDictionary.h) -- inherits its O(n) linear-array lookup (documented
 * there, not repeated here). No NSFastEnumeration, same v1 cut as
 * NSArray.h -- use -allKeys/-objectForKey: or -keysAndObjects... via
 * CFDictionaryApplyFunction-backed helpers instead of `for...in`.
 */
#ifndef FOUNDATION_NSDICTIONARY_H
#define FOUNDATION_NSDICTIONARY_H

#include <Foundation/NSObject.h>

#ifdef __cplusplus
extern "C" {
#endif

@interface NSDictionary : NSObject <NSCopying, NSMutableCopying, NSCoding>

+ (instancetype)dictionary;
+ (instancetype)dictionaryWithObject:(id)object forKey:(id)key;
+ (instancetype)dictionaryWithObjects:(const id [])objects forKeys:(const id [])keys count:(NSUInteger)count;
+ (instancetype)dictionaryWithDictionary:(NSDictionary *)dict;

- (instancetype)initWithObjects:(const id [])objects forKeys:(const id [])keys count:(NSUInteger)count;
- (instancetype)initWithDictionary:(NSDictionary *)dict;

- (NSUInteger)count;
- (id)objectForKey:(id)key;
- (id)objectForKeyedSubscript:(id)key;
- (BOOL)containsObjectForKey:(id)key;
- (NSArray *)allKeys;
- (NSArray *)allValues;

@end

@interface NSMutableDictionary : NSDictionary

+ (instancetype)dictionaryWithCapacity:(NSUInteger)capacity;
- (instancetype)initWithCapacity:(NSUInteger)capacity;

- (void)setObject:(id)anObject forKey:(id)key;
- (void)setObject:(id)anObject forKeyedSubscript:(id)key;
- (void)removeObjectForKey:(id)key;
- (void)removeAllObjects;
- (void)addEntriesFromDictionary:(NSDictionary *)otherDict;

@end

#ifdef __cplusplus
}
#endif

#endif /* FOUNDATION_NSDICTIONARY_H */
