/* Copyright (c) 2026 Vihaan Nathan -- see NSJSONSerialization.h */
#import <Foundation/Foundation.h>
#include "NSCFBridge.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#pragma mark - writer

static BOOL
isJSONBoolean(id value)
{
	return value == (id)kCFBooleanTrue || value == (id)kCFBooleanFalse;
}

static void
appendJSONEscapedString(NSMutableString *out, NSString *s)
{
	const char *utf8 = [s UTF8String];
	if (!utf8) {
		return;
	}
	size_t len = strlen(utf8);
	char *escaped = malloc(len * 6 + 3);	/* worst case: every byte -> \u00XX */
	size_t oi = 0;
	escaped[oi++] = '"';
	for (size_t i = 0; i < len; i++) {
		unsigned char c = (unsigned char)utf8[i];
		switch (c) {
		case '"': escaped[oi++] = '\\'; escaped[oi++] = '"'; break;
		case '\\': escaped[oi++] = '\\'; escaped[oi++] = '\\'; break;
		case '\n': escaped[oi++] = '\\'; escaped[oi++] = 'n'; break;
		case '\r': escaped[oi++] = '\\'; escaped[oi++] = 'r'; break;
		case '\t': escaped[oi++] = '\\'; escaped[oi++] = 't'; break;
		default:
			if (c < 0x20) {
				oi += (size_t)snprintf(escaped + oi, 7, "\\u%04x", c);
			} else {
				escaped[oi++] = (char)c;
			}
			break;
		}
	}
	escaped[oi++] = '"';
	escaped[oi] = '\0';
	CFStringAppendCString((CFMutableStringRef)out, escaped, kCFStringEncodingUTF8);
	free(escaped);
}

static void
writeJSONValue(NSMutableString *out, id value)
{
	if (!value || [value isKindOfClass:[NSNull class]]) {
		[out appendString:[NSString stringWithUTF8String:"null"]];
	} else if (isJSONBoolean(value)) {
		[out appendString:[NSString stringWithUTF8String:([value boolValue] ? "true" : "false")]];
	} else if ([value isKindOfClass:[NSNumber class]]) {
		if (CFNumberIsFloatType((CFNumberRef)value)) {
			char numBuf[64];
			NSCFBridge_formatDouble([value doubleValue], numBuf, sizeof(numBuf));
			[out appendString:[NSString stringWithUTF8String:numBuf]];
		} else {
			[out appendFormat:[NSString stringWithUTF8String:"%ld"], (long)[value longLongValue]];
		}
	} else if ([value isKindOfClass:[NSString class]]) {
		appendJSONEscapedString(out, value);
	} else if ([value isKindOfClass:[NSDictionary class]]) {
		[out appendString:[NSString stringWithUTF8String:"{"]];
		NSArray *keys = [value allKeys];
		NSUInteger n = [keys count];
		for (NSUInteger i = 0; i < n; i++) {
			if (i > 0) {
				[out appendString:[NSString stringWithUTF8String:","]];
			}
			NSString *key = [keys objectAtIndex:i];
			appendJSONEscapedString(out, key);
			[out appendString:[NSString stringWithUTF8String:":"]];
			writeJSONValue(out, [value objectForKey:key]);
		}
		[out appendString:[NSString stringWithUTF8String:"}"]];
	} else if ([value isKindOfClass:[NSArray class]]) {
		[out appendString:[NSString stringWithUTF8String:"["]];
		NSUInteger n = [value count];
		for (NSUInteger i = 0; i < n; i++) {
			if (i > 0) {
				[out appendString:[NSString stringWithUTF8String:","]];
			}
			writeJSONValue(out, [value objectAtIndex:i]);
		}
		[out appendString:[NSString stringWithUTF8String:"]"]];
	}
}

#pragma mark - parser

static void
skipWS(const char **pp)
{
	while (**pp == ' ' || **pp == '\t' || **pp == '\n' || **pp == '\r') {
		(*pp)++;
	}
}

static id parseJSONValue(const char **pp);

static id
parseJSONString(const char **pp)
{
	if (**pp != '"') {
		return nil;
	}
	(*pp)++;
	size_t cap = 64, oi = 0;
	char *buf = malloc(cap);
	while (**pp && **pp != '"') {
		unsigned char c = (unsigned char)**pp;
		if (oi + 6 >= cap) {
			cap *= 2;
			buf = realloc(buf, cap);
		}
		if (c == '\\') {
			(*pp)++;
			switch (**pp) {
			case '"': buf[oi++] = '"'; break;
			case '\\': buf[oi++] = '\\'; break;
			case '/': buf[oi++] = '/'; break;
			case 'b': buf[oi++] = '\b'; break;
			case 'f': buf[oi++] = '\f'; break;
			case 'n': buf[oi++] = '\n'; break;
			case 'r': buf[oi++] = '\r'; break;
			case 't': buf[oi++] = '\t'; break;
			case 'u': {
				/* BMP-only, no surrogate-pair composition -- matches
				 * this tree's CFString's own documented BMP-only
				 * limitation (see CFString.h). */
				char hex[5] = { (*pp)[1], (*pp)[2], (*pp)[3], (*pp)[4], 0 };
				unsigned int cp = (unsigned int)strtoul(hex, NULL, 16);
				*pp += 4;
				if (cp < 0x80) {
					buf[oi++] = (char)cp;
				} else if (cp < 0x800) {
					buf[oi++] = (char)(0xC0 | (cp >> 6));
					buf[oi++] = (char)(0x80 | (cp & 0x3F));
				} else {
					buf[oi++] = (char)(0xE0 | (cp >> 12));
					buf[oi++] = (char)(0x80 | ((cp >> 6) & 0x3F));
					buf[oi++] = (char)(0x80 | (cp & 0x3F));
				}
				break;
			}
			default: buf[oi++] = **pp; break;
			}
			(*pp)++;
		} else {
			buf[oi++] = (char)c;
			(*pp)++;
		}
	}
	if (**pp != '"') {
		free(buf);
		return nil;
	}
	(*pp)++;
	buf[oi] = '\0';
	id s = [[[NSString alloc] initWithUTF8String:buf] autorelease];
	free(buf);
	return s;
}

static id
parseJSONNumber(const char **pp)
{
	const char *start = *pp;
	BOOL isFloat = NO;
	if (**pp == '-') {
		(*pp)++;
	}
	while (isdigit((unsigned char)**pp)) {
		(*pp)++;
	}
	if (**pp == '.') {
		isFloat = YES;
		(*pp)++;
		while (isdigit((unsigned char)**pp)) {
			(*pp)++;
		}
	}
	if (**pp == 'e' || **pp == 'E') {
		isFloat = YES;
		(*pp)++;
		if (**pp == '+' || **pp == '-') {
			(*pp)++;
		}
		while (isdigit((unsigned char)**pp)) {
			(*pp)++;
		}
	}
	size_t len = (size_t)(*pp - start);
	char tmp[64];
	if (len >= sizeof(tmp)) {
		return nil;
	}
	memcpy(tmp, start, len);
	tmp[len] = '\0';
	if (isFloat) {
		return [NSNumber numberWithDouble:strtod(tmp, NULL)];
	}
	return [NSNumber numberWithLongLong:strtoll(tmp, NULL, 10)];
}

static id
parseJSONObject(const char **pp)
{
	(*pp)++;	/* '{' */
	NSMutableDictionary *d = [NSMutableDictionary dictionaryWithCapacity:0];
	skipWS(pp);
	if (**pp == '}') {
		(*pp)++;
		return d;
	}
	for (;;) {
		skipWS(pp);
		id key = parseJSONString(pp);
		if (!key) {
			return nil;
		}
		skipWS(pp);
		if (**pp != ':') {
			return nil;
		}
		(*pp)++;
		skipWS(pp);
		id value = parseJSONValue(pp);
		if (!value) {
			return nil;
		}
		[d setObject:value forKey:key];
		skipWS(pp);
		if (**pp == ',') {
			(*pp)++;
			continue;
		}
		if (**pp == '}') {
			(*pp)++;
			return d;
		}
		return nil;
	}
}

static id
parseJSONArray(const char **pp)
{
	(*pp)++;	/* '[' */
	NSMutableArray *a = [NSMutableArray arrayWithCapacity:0];
	skipWS(pp);
	if (**pp == ']') {
		(*pp)++;
		return a;
	}
	for (;;) {
		skipWS(pp);
		id value = parseJSONValue(pp);
		if (!value) {
			return nil;
		}
		[a addObject:value];
		skipWS(pp);
		if (**pp == ',') {
			(*pp)++;
			continue;
		}
		if (**pp == ']') {
			(*pp)++;
			return a;
		}
		return nil;
	}
}

static id
parseJSONValue(const char **pp)
{
	skipWS(pp);
	switch (**pp) {
	case '{': return parseJSONObject(pp);
	case '[': return parseJSONArray(pp);
	case '"': return parseJSONString(pp);
	case 't':
		if (!strncmp(*pp, "true", 4)) { *pp += 4; return [NSNumber numberWithBool:YES]; }
		return nil;
	case 'f':
		if (!strncmp(*pp, "false", 5)) { *pp += 5; return [NSNumber numberWithBool:NO]; }
		return nil;
	case 'n':
		if (!strncmp(*pp, "null", 4)) { *pp += 4; return [NSNull null]; }
		return nil;
	default:
		if (**pp == '-' || isdigit((unsigned char)**pp)) {
			return parseJSONNumber(pp);
		}
		return nil;
	}
}

#pragma mark - public API

@implementation NSJSONSerialization

+ (BOOL)isValidJSONObject:(id)obj
{
	return [obj isKindOfClass:[NSDictionary class]] || [obj isKindOfClass:[NSArray class]];
}

+ (NSData *)dataWithJSONObject:(id)obj options:(NSJSONWritingOptions)opt error:(NSError **)error
{
	(void)opt;
	if (![self isValidJSONObject:obj]) {
		if (error) {
			*error = [NSError errorWithDomain:NSCocoaErrorDomain code:-1 userInfo:nil];
		}
		return nil;
	}
	NSMutableString *out = [NSMutableString stringWithCapacity:0];
	writeJSONValue(out, obj);
	return [out dataUsingEncoding:NSUTF8StringEncoding];
}

+ (id)JSONObjectWithData:(NSData *)data options:(NSJSONReadingOptions)opt error:(NSError **)error
{
	(void)opt;
	NSUInteger len = [data length];
	char *buf = malloc(len + 1);
	memcpy(buf, [data bytes], len);
	buf[len] = '\0';

	const char *p = buf;
	skipWS(&p);
	id result = nil;
	if (*p == '{' || *p == '[') {	/* top-level fragment not allowed -- see header comment */
		result = parseJSONValue(&p);
	}
	free(buf);
	if (!result && error) {
		*error = [NSError errorWithDomain:NSCocoaErrorDomain code:-1 userInfo:nil];
	}
	return result;
}

@end
