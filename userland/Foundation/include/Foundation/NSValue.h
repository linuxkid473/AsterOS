/* Copyright (c) 2026 Vihaan Nathan
 *
 * NSNumber, toll-free bridged to CFNumber (numeric values) and
 * CFBoolean (boolean values) -- two distinct private concrete classes,
 * NSCFNumber and NSCFBoolean (NSNumber.m), matching real Darwin's own
 * split (__NSCFNumber/__NSCFBoolean are genuinely separate classes
 * there too, not an invented simplification).
 *
 * v1 scope cut: NSValue itself (the general "opaque box for any C
 * scalar/struct/pointer", -getValue:/-objCType) is not implemented --
 * only the numeric-specific NSNumber surface the task's required list
 * asks for. Documented, not an oversight: nothing in this tree currently
 * needs to box arbitrary C structs into an object.
 */
#ifndef FOUNDATION_NSVALUE_H
#define FOUNDATION_NSVALUE_H

#include <Foundation/NSObject.h>

#ifdef __cplusplus
extern "C" {
#endif

@interface NSNumber : NSObject <NSCopying, NSCoding>

+ (instancetype)numberWithInt:(int)value;
+ (instancetype)numberWithUnsignedInt:(unsigned int)value;
+ (instancetype)numberWithInteger:(NSInteger)value;
+ (instancetype)numberWithLong:(long)value;
+ (instancetype)numberWithLongLong:(long long)value;
+ (instancetype)numberWithDouble:(double)value;
+ (instancetype)numberWithFloat:(float)value;
+ (instancetype)numberWithBool:(BOOL)value;
+ (instancetype)numberWithChar:(char)value;

- (instancetype)initWithInt:(int)value;
- (instancetype)initWithInteger:(NSInteger)value;
- (instancetype)initWithDouble:(double)value;
- (instancetype)initWithBool:(BOOL)value;

- (int)intValue;
- (unsigned int)unsignedIntValue;
- (NSInteger)integerValue;
- (long)longValue;
- (long long)longLongValue;
- (double)doubleValue;
- (float)floatValue;
- (BOOL)boolValue;
- (char)charValue;

- (NSString *)stringValue;
- (BOOL)isEqualToNumber:(NSNumber *)other;
- (NSComparisonResult)compare:(NSNumber *)other;

@end

#ifdef __cplusplus
}
#endif

#endif /* FOUNDATION_NSVALUE_H */
