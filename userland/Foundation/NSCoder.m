/* Copyright (c) 2026 Vihaan Nathan -- see NSCoder.h */
#import <Foundation/Foundation.h>

@implementation NSCoder

- (void)encodeObject:(id)object forKey:(NSString *)key { (void)object; (void)key; }
- (id)decodeObjectForKey:(NSString *)key { (void)key; return nil; }

- (void)encodeInt:(int)value forKey:(NSString *)key { (void)value; (void)key; }
- (int)decodeIntForKey:(NSString *)key { (void)key; return 0; }
- (void)encodeInteger:(NSInteger)value forKey:(NSString *)key { (void)value; (void)key; }
- (NSInteger)decodeIntegerForKey:(NSString *)key { (void)key; return 0; }
- (void)encodeDouble:(double)value forKey:(NSString *)key { (void)value; (void)key; }
- (double)decodeDoubleForKey:(NSString *)key { (void)key; return 0.0; }
- (void)encodeBool:(BOOL)value forKey:(NSString *)key { (void)value; (void)key; }
- (BOOL)decodeBoolForKey:(NSString *)key { (void)key; return NO; }

- (BOOL)containsValueForKey:(NSString *)key { (void)key; return NO; }

@end
