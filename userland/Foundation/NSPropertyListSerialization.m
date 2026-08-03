/* Copyright (c) 2026 Vihaan Nathan -- see NSPropertyListSerialization.h */
#import <Foundation/Foundation.h>
#include "NSCFBridge.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#pragma mark - base64

static const char b64_alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static char *
base64_encode(const unsigned char *data, size_t len, size_t *outLen)
{
	size_t olen = ((len + 2) / 3) * 4;
	char *out = malloc(olen + 1);
	size_t oi = 0;
	for (size_t i = 0; i < len; i += 3) {
		unsigned int b0 = data[i];
		unsigned int b1 = (i + 1 < len) ? data[i + 1] : 0;
		unsigned int b2 = (i + 2 < len) ? data[i + 2] : 0;
		unsigned int triple = (b0 << 16) | (b1 << 8) | b2;
		out[oi++] = b64_alphabet[(triple >> 18) & 0x3F];
		out[oi++] = b64_alphabet[(triple >> 12) & 0x3F];
		out[oi++] = (i + 1 < len) ? b64_alphabet[(triple >> 6) & 0x3F] : '=';
		out[oi++] = (i + 2 < len) ? b64_alphabet[triple & 0x3F] : '=';
	}
	out[oi] = '\0';
	*outLen = oi;
	return out;
}

static int
b64_value(char c)
{
	if (c >= 'A' && c <= 'Z') return c - 'A';
	if (c >= 'a' && c <= 'z') return c - 'a' + 26;
	if (c >= '0' && c <= '9') return c - '0' + 52;
	if (c == '+') return 62;
	if (c == '/') return 63;
	return -1;
}

static unsigned char *
base64_decode(const char *s, size_t slen, size_t *outLen)
{
	unsigned char *out = malloc(slen * 3 / 4 + 4);
	size_t oi = 0;
	int vals[4], nvals = 0;
	for (size_t i = 0; i < slen; i++) {
		char c = s[i];
		if (c == '\r' || c == '\n' || c == ' ' || c == '\t' || c == '=') {
			continue;
		}
		int v = b64_value(c);
		if (v < 0) {
			continue;
		}
		vals[nvals++] = v;
		if (nvals == 4) {
			out[oi++] = (unsigned char)((vals[0] << 2) | (vals[1] >> 4));
			out[oi++] = (unsigned char)((vals[1] << 4) | (vals[2] >> 2));
			out[oi++] = (unsigned char)((vals[2] << 6) | vals[3]);
			nvals = 0;
		}
	}
	if (nvals >= 2) {
		out[oi++] = (unsigned char)((vals[0] << 2) | (vals[1] >> 4));
	}
	if (nvals >= 3) {
		out[oi++] = (unsigned char)((vals[1] << 4) | (vals[2] >> 2));
	}
	*outLen = oi;
	return out;
}

#pragma mark - writer

/* Byte-scans the raw UTF-8 -- safe because '&'/'<'/'>' are all
 * single-byte ASCII, and UTF-8 continuation/lead bytes for any
 * multi-byte codepoint are always >= 0x80, so they can never be
 * mistaken for one of these three. Builds one escaped C buffer instead
 * of many small -appendString: calls. */
static void
appendEscaped(NSMutableString *out, NSString *s)
{
	const char *utf8 = [s UTF8String];
	if (!utf8) {
		return;
	}
	size_t len = strlen(utf8);
	char *escaped = malloc(len * 5 + 1);	/* worst case: every byte becomes "&amp;" */
	size_t oi = 0;
	for (size_t i = 0; i < len; i++) {
		switch (utf8[i]) {
		case '&': memcpy(escaped + oi, "&amp;", 5); oi += 5; break;
		case '<': memcpy(escaped + oi, "&lt;", 4); oi += 4; break;
		case '>': memcpy(escaped + oi, "&gt;", 4); oi += 4; break;
		default: escaped[oi++] = utf8[i]; break;
		}
	}
	escaped[oi] = '\0';
	CFStringAppendCString((CFMutableStringRef)out, escaped, kCFStringEncodingUTF8);
	free(escaped);
}

static void
appendIndent(NSMutableString *out, int depth)
{
	for (int i = 0; i < depth; i++) {
		[out appendString:[NSString stringWithUTF8String:"\t"]];
	}
}

static BOOL
isPlistBoolean(id value)
{
	return value == (id)kCFBooleanTrue || value == (id)kCFBooleanFalse;
}

static void
writeValue(NSMutableString *out, id value, int depth)
{
	if ([value isKindOfClass:[NSDictionary class]]) {
		appendIndent(out, depth);
		[out appendString:[NSString stringWithUTF8String:"<dict>\n"]];
		NSArray *keys = [value allKeys];
		NSUInteger n = [keys count];
		for (NSUInteger i = 0; i < n; i++) {
			NSString *key = [keys objectAtIndex:i];
			appendIndent(out, depth + 1);
			[out appendString:[NSString stringWithUTF8String:"<key>"]];
			appendEscaped(out, key);
			[out appendString:[NSString stringWithUTF8String:"</key>\n"]];
			writeValue(out, [value objectForKey:key], depth + 1);
		}
		appendIndent(out, depth);
		[out appendString:[NSString stringWithUTF8String:"</dict>\n"]];
	} else if ([value isKindOfClass:[NSArray class]]) {
		appendIndent(out, depth);
		[out appendString:[NSString stringWithUTF8String:"<array>\n"]];
		NSUInteger n = [value count];
		for (NSUInteger i = 0; i < n; i++) {
			writeValue(out, [value objectAtIndex:i], depth + 1);
		}
		appendIndent(out, depth);
		[out appendString:[NSString stringWithUTF8String:"</array>\n"]];
	} else if (isPlistBoolean(value)) {
		appendIndent(out, depth);
		[out appendString:[NSString stringWithUTF8String:([value boolValue] ? "<true/>\n" : "<false/>\n")]];
	} else if ([value isKindOfClass:[NSNumber class]]) {
		appendIndent(out, depth);
		if (CFNumberIsFloatType((CFNumberRef)value)) {
			char numBuf[64];
			NSCFBridge_formatDouble([value doubleValue], numBuf, sizeof(numBuf));
			[out appendString:[NSString stringWithUTF8String:"<real>"]];
			[out appendString:[NSString stringWithUTF8String:numBuf]];
			[out appendString:[NSString stringWithUTF8String:"</real>\n"]];
		} else {
			[out appendFormat:[NSString stringWithUTF8String:"<integer>%ld</integer>\n"], (long)[value longLongValue]];
		}
	} else if ([value isKindOfClass:[NSString class]]) {
		appendIndent(out, depth);
		[out appendString:[NSString stringWithUTF8String:"<string>"]];
		appendEscaped(out, value);
		[out appendString:[NSString stringWithUTF8String:"</string>\n"]];
	} else if ([value isKindOfClass:[NSData class]]) {
		appendIndent(out, depth);
		size_t b64len;
		char *b64 = base64_encode([value bytes], [value length], &b64len);
		[out appendString:[NSString stringWithUTF8String:"<data>"]];
		[out appendString:[NSString stringWithUTF8String:b64]];
		[out appendString:[NSString stringWithUTF8String:"</data>\n"]];
		free(b64);
	} else if ([value isKindOfClass:[NSDate class]]) {
		appendIndent(out, depth);
		time_t secs = (time_t)[value timeIntervalSince1970];
		struct tm tm;
		gmtime_r(&secs, &tm);
		char buf[32];
		snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ",
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
		[out appendString:[NSString stringWithUTF8String:"<date>"]];
		[out appendString:[NSString stringWithUTF8String:buf]];
		[out appendString:[NSString stringWithUTF8String:"</date>\n"]];
	}
	/* Anything else (NSNull, custom objects, ...) is silently skipped --
	 * no plist representation exists, see this header's own comment. */
}

#pragma mark - parser (adapted from userland/launchd/plist.c)

static void
skip_ws(const char **pp)
{
	const char *p = *pp;
	for (;;) {
		while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
			p++;
		}
		if (p[0] == '<' && p[1] == '!' && p[2] == '-' && p[3] == '-') {
			const char *end = strstr(p, "-->");
			if (!end) {
				break;
			}
			p = end + 3;
			continue;
		}
		break;
	}
	*pp = p;
}

static int
consume(const char **pp, const char *lit)
{
	skip_ws(pp);
	size_t n = strlen(lit);
	if (strncmp(*pp, lit, n) != 0) {
		return 0;
	}
	*pp += n;
	return 1;
}

static Boolean
peek(const char **pp, const char *lit)
{
	const char *save = *pp;
	int r = consume(pp, lit);
	*pp = save;
	return r ? true : false;
}

/* Malloc'd copy of the raw text up to close_tag, with &amp;/&lt;/&gt;/
 * &apos;/&quot; unescaped -- launchd's own plist.c doesn't need this
 * (its plists never contain escaped text in practice), but a general
 * plist reader has to. */
static char *
text_until(const char **pp, const char *close_tag)
{
	const char *end = strstr(*pp, close_tag);
	if (!end) {
		return NULL;
	}
	size_t rawlen = (size_t)(end - *pp);
	char *out = malloc(rawlen + 1);
	size_t oi = 0;
	for (size_t i = 0; i < rawlen; ) {
		const char *p = *pp + i;
		if (!strncmp(p, "&amp;", 5)) { out[oi++] = '&'; i += 5; }
		else if (!strncmp(p, "&lt;", 4)) { out[oi++] = '<'; i += 4; }
		else if (!strncmp(p, "&gt;", 4)) { out[oi++] = '>'; i += 4; }
		else if (!strncmp(p, "&apos;", 6)) { out[oi++] = '\''; i += 6; }
		else if (!strncmp(p, "&quot;", 6)) { out[oi++] = '"'; i += 6; }
		else { out[oi++] = p[0]; i++; }
	}
	out[oi] = '\0';
	*pp = end + strlen(close_tag);
	return out;
}

static id parseValue(const char **pp);

static id
parseDict(const char **pp)
{
	if (!consume(pp, "<dict>")) {
		return nil;
	}
	NSMutableDictionary *d = [NSMutableDictionary dictionaryWithCapacity:0];
	for (;;) {
		if (consume(pp, "</dict>")) {
			return d;
		}
		if (!consume(pp, "<key>")) {
			return nil;
		}
		char *keyText = text_until(pp, "</key>");
		if (!keyText) {
			return nil;
		}
		NSString *key = [[[NSString alloc] initWithUTF8String:keyText] autorelease];
		free(keyText);
		id value = parseValue(pp);
		if (!value) {
			return nil;
		}
		[d setObject:value forKey:key];
	}
}

static id
parseArray(const char **pp)
{
	if (!consume(pp, "<array>")) {
		return nil;
	}
	NSMutableArray *a = [NSMutableArray arrayWithCapacity:0];
	for (;;) {
		if (consume(pp, "</array>")) {
			return a;
		}
		id value = parseValue(pp);
		if (!value) {
			return nil;
		}
		[a addObject:value];
	}
}

static id
parseValue(const char **pp)
{
	skip_ws(pp);
	if (peek(pp, "<dict>")) {
		return parseDict(pp);
	}
	if (peek(pp, "<array>")) {
		return parseArray(pp);
	}
	if (consume(pp, "<true/>")) {
		return [NSNumber numberWithBool:YES];
	}
	if (consume(pp, "<false/>")) {
		return [NSNumber numberWithBool:NO];
	}
	if (consume(pp, "<string>")) {
		char *text = text_until(pp, "</string>");
		if (!text) {
			return nil;
		}
		id s = [[[NSString alloc] initWithUTF8String:text] autorelease];
		free(text);
		return s;
	}
	if (consume(pp, "<integer>")) {
		char *text = text_until(pp, "</integer>");
		if (!text) {
			return nil;
		}
		long long v = strtoll(text, NULL, 10);
		free(text);
		return [NSNumber numberWithLongLong:v];
	}
	if (consume(pp, "<real>")) {
		char *text = text_until(pp, "</real>");
		if (!text) {
			return nil;
		}
		double v = strtod(text, NULL);
		free(text);
		return [NSNumber numberWithDouble:v];
	}
	if (consume(pp, "<data>")) {
		char *text = text_until(pp, "</data>");
		if (!text) {
			return nil;
		}
		size_t declen;
		unsigned char *bytes = base64_decode(text, strlen(text), &declen);
		free(text);
		id d = [NSData dataWithBytes:bytes length:declen];
		free(bytes);
		return d;
	}
	if (consume(pp, "<date>")) {
		char *text = text_until(pp, "</date>");
		if (!text) {
			return nil;
		}
		struct tm tm;
		memset(&tm, 0, sizeof(tm));
		int y, mo, d, h, mi, s;
		if (sscanf(text, "%d-%d-%dT%d:%d:%dZ", &y, &mo, &d, &h, &mi, &s) != 6) {
			free(text);
			return nil;
		}
		free(text);
		tm.tm_year = y - 1900;
		tm.tm_mon = mo - 1;
		tm.tm_mday = d;
		tm.tm_hour = h;
		tm.tm_min = mi;
		tm.tm_sec = s;
		time_t secs = mktime(&tm);
		return [NSDate dateWithTimeIntervalSince1970:(NSTimeInterval)secs];
	}
	return nil;	/* not a tag this parser recognizes -- see header comment */
}

#pragma mark - public API

@implementation NSPropertyListSerialization

+ (NSData *)dataWithPropertyList:(id)plist format:(NSPropertyListFormat)format options:(NSUInteger)opt error:(NSError **)error
{
	(void)opt;
	if (format != NSPropertyListXMLFormat_v1_0) {
		if (error) {
			*error = [NSError errorWithDomain:NSCocoaErrorDomain code:-1 userInfo:nil];
		}
		return nil;	/* NSPropertyListBinaryFormat_v1_0 not implemented -- see header comment */
	}
	NSMutableString *out = [NSMutableString stringWithCapacity:0];
	[out appendString:[NSString stringWithUTF8String:"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"]];
	[out appendString:[NSString stringWithUTF8String:"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"]];
	[out appendString:[NSString stringWithUTF8String:"<plist version=\"1.0\">\n"]];
	writeValue(out, plist, 0);
	[out appendString:[NSString stringWithUTF8String:"</plist>\n"]];
	return [out dataUsingEncoding:NSUTF8StringEncoding];
}

+ (id)propertyListWithData:(NSData *)data options:(NSUInteger)opt format:(NSPropertyListFormat *)format error:(NSError **)error
{
	(void)opt;
	NSUInteger len = [data length];
	char *buf = malloc(len + 1);
	memcpy(buf, [data bytes], len);
	buf[len] = '\0';

	const char *p = strstr(buf, "<plist");
	/* strstr only finds where the *opening* tag starts -- parseValue
	 * expects to see a value-starting tag (<dict>/<array>/...) next, so
	 * the "<plist version=\"1.0\">" tag itself (attributes and all) has
	 * to be skipped past first. Caught live: without this, parseValue
	 * silently fails to match anything against "<plist version=..." and
	 * returns nil on every real plist, never reached by any earlier
	 * milestone's testing since nothing round-tripped through the
	 * parser this deep until the full test suite ran end-to-end. */
	if (p) {
		p = strchr(p, '>');
		if (p) {
			p++;
		}
	}
	if (!p) {
		free(buf);
		if (error) {
			*error = [NSError errorWithDomain:NSCocoaErrorDomain code:-1 userInfo:nil];
		}
		return nil;
	}
	id result = parseValue(&p);
	free(buf);
	if (!result) {
		if (error) {
			*error = [NSError errorWithDomain:NSCocoaErrorDomain code:-1 userInfo:nil];
		}
		return nil;
	}
	if (format) {
		*format = NSPropertyListXMLFormat_v1_0;
	}
	return result;
}

+ (BOOL)propertyList:(id)plist isValidForFormat:(NSPropertyListFormat)format
{
	if (format != NSPropertyListXMLFormat_v1_0) {
		return NO;
	}
	return [plist isKindOfClass:[NSDictionary class]] || [plist isKindOfClass:[NSArray class]];
}

@end
