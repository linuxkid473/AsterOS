/* Copyright (c) 2026 Vihaan Nathan
 *
 * NSCFString is the one concrete class backing the whole NSString/
 * NSMutableString cluster in this tree (see NSString.h). It declares no
 * ivars of its own: its real storage is CFString's struct __CFString,
 * entirely allocated and managed by CoreFoundation -- every instance is
 * created by calling straight into a CFStringCreate* function and
 * casting the result, never by the generic NSObject +alloc path (which
 * would allocate a block sized by this class's -- empty -- declared
 * ivars, the wrong size entirely). +alloc is overridden on NSString
 * itself for exactly this reason: it returns a valid, empty, correctly
 * CF-backed placeholder that every -initWith...: replaces (releasing
 * the placeholder first), the standard "an init method may return an
 * object other than self" Cocoa class-cluster pattern.
 *
 * Compiled without ARC, same reasoning as NSObject.m/Root.m: -retain/
 * -release/-dealloc here define real bridged memory-management
 * semantics, not something ARC should insert calls around.
 */
#import <Foundation/Foundation.h>
#include "NSCFBridge.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

@interface NSCFString : NSMutableString
@end

@implementation NSString

+ (instancetype)string
{
	return [[[NSCFString alloc] init] autorelease];
}

+ (instancetype)stringWithUTF8String:(const char *)cStr
{
	return [[[NSCFString alloc] initWithUTF8String:cStr] autorelease];
}

+ (instancetype)stringWithString:(NSString *)aString
{
	return [[[NSCFString alloc] initWithString:aString] autorelease];
}

+ (instancetype)stringWithFormat:(NSString *)format, ...
{
	va_list args;
	va_start(args, format);
	CFStringRef s = CFStringCreateWithFormatAndArguments(kCFAllocatorDefault, NULL, (CFStringRef)format, args);
	va_end(args);
	return [(id)s autorelease];
}

+ (instancetype)stringWithContentsOfFile:(NSString *)path
{
	FILE *f = fopen([path UTF8String], "rb");
	if (!f) {
		return nil;
	}
	fseek(f, 0, SEEK_END);
	long len = ftell(f);
	fseek(f, 0, SEEK_SET);
	char *buf = malloc((size_t)len + 1);
	size_t n = fread(buf, 1, (size_t)len, f);
	buf[n] = '\0';
	fclose(f);
	NSString *result = [NSString stringWithUTF8String:buf];
	free(buf);
	return result;
}

+ (id)alloc
{
	/* Placeholder: a valid, empty, CF-backed, mutable instance --
	 * every real -initWith...: below releases this and returns a
	 * freshly created replacement instead. See this file's header
	 * comment. */
	return (id)CFStringCreateMutable(kCFAllocatorDefault, 0);
}

- (instancetype)init
{
	return self;
}

- (instancetype)initWithUTF8String:(const char *)cStr
{
	[self release];
	return (id)CFStringCreateWithCString(kCFAllocatorDefault, cStr, kCFStringEncodingUTF8);
}

- (instancetype)initWithString:(NSString *)aString
{
	[self release];
	return (id)CFStringCreateCopy(kCFAllocatorDefault, (CFStringRef)aString);
}

- (instancetype)initWithFormat:(NSString *)format, ...
{
	[self release];
	va_list args;
	va_start(args, format);
	CFStringRef s = CFStringCreateWithFormatAndArguments(kCFAllocatorDefault, NULL, (CFStringRef)format, args);
	va_end(args);
	return (id)s;
}

- (instancetype)initWithBytes:(const void *)bytes length:(NSUInteger)length encoding:(NSStringEncoding)encoding
{
	(void)encoding;
	[self release];
	return (id)CFStringCreateWithBytes(kCFAllocatorDefault, bytes, (CFIndex)length, kCFStringEncodingUTF8, false);
}

- (instancetype)initWithData:(NSData *)data encoding:(NSStringEncoding)encoding
{
	(void)encoding;
	[self release];
	return (id)CFStringCreateWithBytes(kCFAllocatorDefault, [data bytes], (CFIndex)[data length], kCFStringEncodingUTF8, false);
}

- (NSString *)description
{
	return self;
}

- (id)copyWithZone:(NSZone *)zone
{
	(void)zone;
	return (id)CFStringCreateCopy(kCFAllocatorDefault, (CFStringRef)self);
}

- (id)mutableCopyWithZone:(NSZone *)zone
{
	(void)zone;
	return (id)CFStringCreateMutableCopy(kCFAllocatorDefault, 0, (CFStringRef)self);
}

@end

@implementation NSMutableString

+ (instancetype)stringWithCapacity:(NSUInteger)capacity
{
	return [[[NSCFString alloc] initWithCapacity:capacity] autorelease];
}

- (instancetype)initWithCapacity:(NSUInteger)capacity
{
	[self release];
	return (id)CFStringCreateMutable(kCFAllocatorDefault, (CFIndex)capacity);
}

@end

@implementation NSCFString

- (id)retain { return NSCFBridge_retain(self); }
- (oneway void)release { NSCFBridge_release(self); }
- (NSUInteger)retainCount { return NSCFBridge_retainCount(self); }
- (BOOL)isEqual:(id)other { return NSCFBridge_isEqual(self, other); }
- (NSUInteger)hash { return NSCFBridge_hash(self); }
- (void)dealloc { NSCFBridge_deallocGuard(self); }

- (NSUInteger)length
{
	return (NSUInteger)CFStringGetLength((CFStringRef)self);
}

- (unichar)characterAtIndex:(NSUInteger)index
{
	return (unichar)CFStringGetCharacterAtIndex((CFStringRef)self, (CFIndex)index);
}

- (void)getCharacters:(unichar *)buffer range:(NSRange)range
{
	CFStringGetCharacters((CFStringRef)self, CFRangeMake((CFIndex)range.location, (CFIndex)range.length), buffer);
}

- (const char *)UTF8String
{
	return CFStringGetCStringPtr((CFStringRef)self, kCFStringEncodingUTF8);
}

- (BOOL)getCString:(char *)buffer maxLength:(NSUInteger)maxLength encoding:(NSStringEncoding)encoding
{
	(void)encoding;
	return CFStringGetCString((CFStringRef)self, buffer, (CFIndex)maxLength, kCFStringEncodingUTF8) ? YES : NO;
}

- (BOOL)isEqualToString:(NSString *)other
{
	if (!other) {
		return NO;
	}
	return CFEqual((CFTypeRef)self, (CFTypeRef)other) ? YES : NO;
}

- (NSComparisonResult)compare:(NSString *)other
{
	return (NSComparisonResult)CFStringCompare((CFStringRef)self, (CFStringRef)other, 0);
}

- (NSComparisonResult)caseInsensitiveCompare:(NSString *)other
{
	return (NSComparisonResult)CFStringCompare((CFStringRef)self, (CFStringRef)other, kCFCompareCaseInsensitive);
}

- (BOOL)hasPrefix:(NSString *)prefix
{
	return CFStringHasPrefix((CFStringRef)self, (CFStringRef)prefix) ? YES : NO;
}

- (BOOL)hasSuffix:(NSString *)suffix
{
	return CFStringHasSuffix((CFStringRef)self, (CFStringRef)suffix) ? YES : NO;
}

- (NSRange)rangeOfString:(NSString *)needle
{
	CFRange r;
	if (!CFStringFind((CFStringRef)self, (CFStringRef)needle, 0, &r)) {
		return NSMakeRange(NSIntegerMax, 0);
	}
	return NSMakeRange((NSUInteger)r.location, (NSUInteger)r.length);
}

- (BOOL)containsString:(NSString *)needle
{
	CFRange r;
	return CFStringFind((CFStringRef)self, (CFStringRef)needle, 0, &r) ? YES : NO;
}

- (NSString *)substringFromIndex:(NSUInteger)from
{
	NSUInteger len = [self length];
	return [self substringWithRange:NSMakeRange(from, len - from)];
}

- (NSString *)substringToIndex:(NSUInteger)to
{
	return [self substringWithRange:NSMakeRange(0, to)];
}

- (NSString *)substringWithRange:(NSRange)range
{
	unichar *buf = malloc(sizeof(unichar) * (range.length ? range.length : 1));
	[self getCharacters:buf range:range];
	/* Round-trip through UTF-8: this tree's CFString is UTF-8-backed
	 * (see CFString.h), so re-encode the BMP-only unichars back down
	 * rather than adding a second UTF-16-based creation path. */
	char *utf8 = malloc(range.length * 4 + 1);
	size_t o = 0;
	for (NSUInteger i = 0; i < range.length; i++) {
		unsigned int cp = buf[i];
		if (cp < 0x80) {
			utf8[o++] = (char)cp;
		} else if (cp < 0x800) {
			utf8[o++] = (char)(0xC0 | (cp >> 6));
			utf8[o++] = (char)(0x80 | (cp & 0x3F));
		} else {
			utf8[o++] = (char)(0xE0 | (cp >> 12));
			utf8[o++] = (char)(0x80 | ((cp >> 6) & 0x3F));
			utf8[o++] = (char)(0x80 | (cp & 0x3F));
		}
	}
	utf8[o] = '\0';
	NSString *result = (id)CFStringCreateWithBytes(kCFAllocatorDefault, (const UInt8 *)utf8, (CFIndex)o, kCFStringEncodingUTF8, false);
	free(buf);
	free(utf8);
	return [result autorelease];
}

- (NSString *)stringByAppendingString:(NSString *)other
{
	CFMutableStringRef m = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, (CFStringRef)self);
	CFStringAppend(m, (CFStringRef)other);
	return [(id)m autorelease];
}

- (NSString *)stringByAppendingFormat:(NSString *)format, ...
{
	va_list args;
	va_start(args, format);
	CFStringRef piece = CFStringCreateWithFormatAndArguments(kCFAllocatorDefault, NULL, (CFStringRef)format, args);
	va_end(args);
	NSString *result = [self stringByAppendingString:(id)piece];
	CFRelease(piece);
	return result;
}

- (NSString *)stringByTrimmingCharactersInSet:(id)set
{
	(void)set;
	/* v1: trimming-set support not implemented -- no NSCharacterSet in
	 * this tree yet (documented gap, see TODO.md). */
	return self;
}

- (NSArray *)componentsSeparatedByString:(NSString *)separator
{
	CFArrayRef pieces = CFStringCreateArrayBySeparatingStrings(kCFAllocatorDefault, (CFStringRef)self, (CFStringRef)separator);
	return [(id)pieces autorelease];
}

- (double)doubleValue
{
	return strtod([self UTF8String] ? [self UTF8String] : "", NULL);
}

- (float)floatValue
{
	return (float)[self doubleValue];
}

- (NSInteger)integerValue
{
	const char *s = [self UTF8String];
	return s ? strtol(s, NULL, 10) : 0;
}

- (int)intValue
{
	return (int)[self integerValue];
}

- (BOOL)boolValue
{
	const char *s = [self UTF8String];
	if (!s) {
		return NO;
	}
	while (*s == ' ' || *s == '\t') {
		s++;
	}
	if (*s == '-' || *s == '+') {
		s++;
	}
	if (*s >= '1' && *s <= '9') {
		return YES;
	}
	return (strncasecmp(s, "yes", 3) == 0 || strncasecmp(s, "true", 4) == 0) ? YES : NO;
}

- (NSData *)dataUsingEncoding:(NSStringEncoding)encoding
{
	(void)encoding;
	CFIndex len = CFStringGetLength((CFStringRef)self);
	CFIndex maxBytes = CFStringGetMaximumSizeForEncoding(len, kCFStringEncodingUTF8);
	UInt8 *buf = malloc((size_t)maxBytes);
	CFStringGetCString((CFStringRef)self, (char *)buf, maxBytes + 1, kCFStringEncodingUTF8);
	CFDataRef d = CFDataCreate(kCFAllocatorDefault, buf, (CFIndex)strlen((char *)buf));
	free(buf);
	return [(id)d autorelease];
}

/* ---- NSMutableString methods ---- */

- (void)appendString:(NSString *)other
{
	CFStringAppend((CFMutableStringRef)self, (CFStringRef)other);
}

- (void)appendFormat:(NSString *)format, ...
{
	va_list args;
	va_start(args, format);
	CFStringRef piece = CFStringCreateWithFormatAndArguments(kCFAllocatorDefault, NULL, (CFStringRef)format, args);
	va_end(args);
	CFStringAppend((CFMutableStringRef)self, piece);
	CFRelease(piece);
}

- (void)insertString:(NSString *)str atIndex:(NSUInteger)index
{
	CFStringInsert((CFMutableStringRef)self, (CFIndex)index, (CFStringRef)str);
}

- (void)deleteCharactersInRange:(NSRange)range
{
	CFStringDelete((CFMutableStringRef)self, CFRangeMake((CFIndex)range.location, (CFIndex)range.length));
}

- (void)replaceCharactersInRange:(NSRange)range withString:(NSString *)replacement
{
	CFStringDelete((CFMutableStringRef)self, CFRangeMake((CFIndex)range.location, (CFIndex)range.length));
	CFStringInsert((CFMutableStringRef)self, (CFIndex)range.location, (CFStringRef)replacement);
}

- (void)setString:(NSString *)aString
{
	CFStringDelete((CFMutableStringRef)self, CFRangeMake(0, CFStringGetLength((CFStringRef)self)));
	CFStringAppend((CFMutableStringRef)self, (CFStringRef)aString);
}

@end
