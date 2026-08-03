/* Copyright (c) 2026 Vihaan Nathan
 *
 * Abstract base, real Cocoa-style: every primitive here has a harmless
 * default (encode = no-op, decode = zero/nil) so a concrete subclass
 * only needs to override the direction it actually implements
 * (NSKeyedArchiver overrides encode*, NSKeyedUnarchiver overrides
 * decode*) -- matches real NSCoder's own design, not a shortcut.
 */
#ifndef FOUNDATION_NSCODER_H
#define FOUNDATION_NSCODER_H

#include <Foundation/NSObject.h>

#ifdef __cplusplus
extern "C" {
#endif

@interface NSCoder : NSObject

- (void)encodeObject:(id)object forKey:(NSString *)key;
- (id)decodeObjectForKey:(NSString *)key;

- (void)encodeInt:(int)value forKey:(NSString *)key;
- (int)decodeIntForKey:(NSString *)key;
- (void)encodeInteger:(NSInteger)value forKey:(NSString *)key;
- (NSInteger)decodeIntegerForKey:(NSString *)key;
- (void)encodeDouble:(double)value forKey:(NSString *)key;
- (double)decodeDoubleForKey:(NSString *)key;
- (void)encodeBool:(BOOL)value forKey:(NSString *)key;
- (BOOL)decodeBoolForKey:(NSString *)key;

- (BOOL)containsValueForKey:(NSString *)key;

@end

#ifdef __cplusplus
}
#endif

#endif /* FOUNDATION_NSCODER_H */
