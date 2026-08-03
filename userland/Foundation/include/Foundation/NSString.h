/* Copyright (c) 2026 Vihaan Nathan
 *
 * Toll-free bridged to CFString (userland/CoreFoundation/CFString.h) --
 * an NSString* and a CFStringRef are the same object; -isEqualToString:/
 * -hash/-retain/-release answer identically whether reached through CF
 * or NS API. v1 simplification: one concrete backing class (NSCFString,
 * NSString.m) for the whole cluster, not full isa-swizzling subclass
 * support -- documented, not silently incomplete (see TODO.md).
 * Inherits CFString's own v1 tradeoff: UTF-8 storage, BMP-only UniChar
 * decoding (see CFString.h).
 */
#ifndef FOUNDATION_NSSTRING_H
#define FOUNDATION_NSSTRING_H

#include <Foundation/NSObject.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

@class NSArray;
@class NSData;

typedef unsigned short unichar;

typedef NSUInteger NSStringCompareOptions;
enum {
	NSCaseInsensitiveSearch = 1,
	NSBackwardsSearch = 4,
	NSAnchoredSearch = 8,
};

@interface NSString : NSObject <NSCopying, NSMutableCopying, NSCoding>

+ (instancetype)string;
+ (instancetype)stringWithUTF8String:(const char *)cStr;
+ (instancetype)stringWithString:(NSString *)aString;
+ (instancetype)stringWithFormat:(NSString *)format, ...;
+ (instancetype)stringWithContentsOfFile:(NSString *)path;

- (instancetype)init;
- (instancetype)initWithUTF8String:(const char *)cStr;
- (instancetype)initWithString:(NSString *)aString;
- (instancetype)initWithFormat:(NSString *)format, ...;
- (instancetype)initWithBytes:(const void *)bytes length:(NSUInteger)length encoding:(NSStringEncoding)encoding;
- (instancetype)initWithData:(NSData *)data encoding:(NSStringEncoding)encoding;

- (NSUInteger)length;
- (unichar)characterAtIndex:(NSUInteger)index;
- (void)getCharacters:(unichar *)buffer range:(NSRange)range;

- (const char *)UTF8String;
- (BOOL)getCString:(char *)buffer maxLength:(NSUInteger)maxLength encoding:(NSStringEncoding)encoding;

- (BOOL)isEqualToString:(NSString *)other;
- (NSComparisonResult)compare:(NSString *)other;
- (NSComparisonResult)caseInsensitiveCompare:(NSString *)other;
- (BOOL)hasPrefix:(NSString *)prefix;
- (BOOL)hasSuffix:(NSString *)suffix;
- (NSRange)rangeOfString:(NSString *)needle;
- (BOOL)containsString:(NSString *)needle;

- (NSString *)substringFromIndex:(NSUInteger)from;
- (NSString *)substringToIndex:(NSUInteger)to;
- (NSString *)substringWithRange:(NSRange)range;
- (NSString *)stringByAppendingString:(NSString *)other;
- (NSString *)stringByAppendingFormat:(NSString *)format, ...;
- (NSString *)stringByTrimmingCharactersInSet:(id)set;

- (NSArray *)componentsSeparatedByString:(NSString *)separator;

- (double)doubleValue;
- (float)floatValue;
- (NSInteger)integerValue;
- (int)intValue;
- (BOOL)boolValue;

- (NSData *)dataUsingEncoding:(NSStringEncoding)encoding;

@end

@interface NSMutableString : NSString

+ (instancetype)stringWithCapacity:(NSUInteger)capacity;
- (instancetype)initWithCapacity:(NSUInteger)capacity;

- (void)appendString:(NSString *)other;
- (void)appendFormat:(NSString *)format, ...;
- (void)insertString:(NSString *)str atIndex:(NSUInteger)index;
- (void)deleteCharactersInRange:(NSRange)range;
- (void)replaceCharactersInRange:(NSRange)range withString:(NSString *)replacement;
- (void)setString:(NSString *)aString;

@end

#ifdef __cplusplus
}
#endif

#endif /* FOUNDATION_NSSTRING_H */
