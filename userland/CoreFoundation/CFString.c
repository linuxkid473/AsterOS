/* Copyright (c) 2026 Vihaan Nathan
 *
 * See CFString.h for the UTF-8-instead-of-UTF-16 v1 tradeoff. Internally
 * every string is a NUL-terminated UTF-8 byte buffer; CFStringGetLength()/
 * CFStringGetCharacterAtIndex() decode it on the fly to answer in
 * (approximated, BMP-only) UTF-16 code-unit terms, same spirit as this
 * tree's other "real semantics, simplified storage" choices.
 */
#include "CFInternal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <pthread.h>

struct __CFString {
	CFRuntimeBase base;
	char *buffer;		/* NUL-terminated UTF-8 bytes */
	CFIndex byteLength;	/* excludes the NUL */
	CFIndex capacity;	/* allocated size of buffer, mutable strings only */
	CFIndex maxLength;	/* 0 = unbounded; mutable strings only, see CFStringCreateMutable */
	Boolean isMutable;
};

static CFTypeID g_stringTypeID;
static pthread_once_t g_stringOnce = PTHREAD_ONCE_INIT;

static void stringFinalize(CFTypeRef cf)
{
	struct __CFString *s = (struct __CFString *)cf;
	free(s->buffer);
}

static Boolean stringEqual(CFTypeRef cf1, CFTypeRef cf2)
{
	const struct __CFString *a = cf1, *b = cf2;
	return a->byteLength == b->byteLength && memcmp(a->buffer, b->buffer, (size_t)a->byteLength) == 0;
}

static CFHashCode stringHash(CFTypeRef cf)
{
	const struct __CFString *s = cf;
	/* FNV-1a -- an internal implementation detail, doesn't need to match
	 * real CF's hash, only needs to be stable and well-distributed. */
	CFHashCode h = 2166136261u;
	for (CFIndex i = 0; i < s->byteLength; i++) {
		h ^= (unsigned char)s->buffer[i];
		h *= 16777619u;
	}
	return h;
}

static CFStringRef stringCopyDesc(CFTypeRef cf)
{
	const struct __CFString *s = cf;
	return CFStringCreateWithCString(kCFAllocatorDefault, s->buffer, kCFStringEncodingUTF8);
}

static void stringInit(void)
{
	static const CFRuntimeClass cls = {
		.className = "CFString",
		.finalize = stringFinalize,
		.equal = stringEqual,
		.hash = stringHash,
		.copyFormattingDesc = stringCopyDesc,
	};
	g_stringTypeID = _CFRuntimeRegisterClass(&cls);
}

CFTypeID CFStringGetTypeID(void)
{
	pthread_once(&g_stringOnce, stringInit);
	return g_stringTypeID;
}

/* ---- construction ---- */

static struct __CFString *stringAlloc(CFAllocatorRef allocator)
{
	CFStringGetTypeID();
	return (struct __CFString *)_CFRuntimeCreateInstance(allocator, g_stringTypeID, sizeof(struct __CFString) - sizeof(CFRuntimeBase));
}

static struct __CFString *stringCreateFromBytes(CFAllocatorRef allocator, const char *bytes, CFIndex len, Boolean mutable)
{
	struct __CFString *s = stringAlloc(allocator);
	s->byteLength = len;
	s->capacity = len + 1;
	s->buffer = malloc((size_t)s->capacity);
	memcpy(s->buffer, bytes, (size_t)len);
	s->buffer[len] = '\0';
	s->isMutable = mutable;
	return s;
}

CFStringRef CFStringCreateWithCString(CFAllocatorRef alloc, const char *cStr, CFStringEncoding encoding)
{
	(void)encoding;	/* v1: everything is treated as raw bytes -- see CFString.h */
	return (CFStringRef)stringCreateFromBytes(alloc, cStr, (CFIndex)strlen(cStr), false);
}

CFStringRef CFStringCreateWithBytes(CFAllocatorRef alloc, const UInt8 *bytes, CFIndex numBytes, CFStringEncoding encoding, Boolean isExternalRepresentation)
{
	(void)encoding;
	(void)isExternalRepresentation;
	return (CFStringRef)stringCreateFromBytes(alloc, (const char *)bytes, numBytes, false);
}

CFStringRef CFStringCreateCopy(CFAllocatorRef alloc, CFStringRef theString)
{
	const struct __CFString *src = (const struct __CFString *)theString;
	return (CFStringRef)stringCreateFromBytes(alloc, src->buffer, src->byteLength, false);
}

CFMutableStringRef CFStringCreateMutable(CFAllocatorRef alloc, CFIndex maxLength)
{
	struct __CFString *s = stringAlloc(alloc);
	s->capacity = 16;
	s->buffer = malloc((size_t)s->capacity);
	s->buffer[0] = '\0';
	s->byteLength = 0;
	s->maxLength = maxLength;
	s->isMutable = true;
	return (CFMutableStringRef)s;
}

CFMutableStringRef CFStringCreateMutableCopy(CFAllocatorRef alloc, CFIndex maxLength, CFStringRef theString)
{
	const struct __CFString *src = (const struct __CFString *)theString;
	CFMutableStringRef m = CFStringCreateMutable(alloc, maxLength);
	CFStringAppend(m, (CFStringRef)src);
	return m;
}

/* ---- mutation core ---- */

static void stringAppendBytes(struct __CFString *s, const char *bytes, CFIndex len)
{
	if (s->maxLength > 0 && s->byteLength + len > s->maxLength)
		len = s->maxLength - s->byteLength;
	if (len <= 0)
		return;
	CFIndex needed = s->byteLength + len + 1;
	if (needed > s->capacity) {
		while (s->capacity < needed)
			s->capacity *= 2;
		s->buffer = realloc(s->buffer, (size_t)s->capacity);
	}
	memcpy(s->buffer + s->byteLength, bytes, (size_t)len);
	s->byteLength += len;
	s->buffer[s->byteLength] = '\0';
}

void CFStringAppend(CFMutableStringRef theString, CFStringRef appendedString)
{
	const struct __CFString *src = (const struct __CFString *)appendedString;
	stringAppendBytes((struct __CFString *)theString, src->buffer, src->byteLength);
}

void CFStringAppendCString(CFMutableStringRef theString, const char *cStr, CFStringEncoding encoding)
{
	(void)encoding;
	stringAppendBytes((struct __CFString *)theString, cStr, (CFIndex)strlen(cStr));
}

/* ---- UTF-8 decode helpers ---- */

static uint32_t utf8Decode(const char *p, int *consumed)
{
	unsigned char c = (unsigned char)p[0];
	if (c < 0x80) {
		*consumed = 1;
		return c;
	} else if ((c & 0xE0) == 0xC0 && p[1]) {
		*consumed = 2;
		return ((uint32_t)(c & 0x1F) << 6) | (uint32_t)(p[1] & 0x3F);
	} else if ((c & 0xF0) == 0xE0 && p[1] && p[2]) {
		*consumed = 3;
		return ((uint32_t)(c & 0x0F) << 12) | ((uint32_t)(p[1] & 0x3F) << 6) | (uint32_t)(p[2] & 0x3F);
	} else if ((c & 0xF8) == 0xF0 && p[1] && p[2] && p[3]) {
		*consumed = 4;
		return ((uint32_t)(c & 0x07) << 18) | ((uint32_t)(p[1] & 0x3F) << 12) | ((uint32_t)(p[2] & 0x3F) << 6) | (uint32_t)(p[3] & 0x3F);
	}
	*consumed = 1;
	return c;
}

/* Byte offset of the start of the idx'th decoded codepoint. Codepoints
 * outside the BMP are counted as one unit here (not a real UTF-16
 * surrogate pair) -- see this file's header comment. */
static CFIndex charIndexToByteOffset(const struct __CFString *s, CFIndex charIdx)
{
	CFIndex off = 0, ci = 0;
	while (ci < charIdx && off < s->byteLength) {
		int consumed;
		utf8Decode(s->buffer + off, &consumed);
		off += consumed;
		ci++;
	}
	return off;
}

CFIndex CFStringGetLength(CFStringRef theString)
{
	const struct __CFString *s = (const struct __CFString *)theString;
	CFIndex off = 0, count = 0;
	while (off < s->byteLength) {
		int consumed;
		utf8Decode(s->buffer + off, &consumed);
		off += consumed;
		count++;
	}
	return count;
}

UniChar CFStringGetCharacterAtIndex(CFStringRef theString, CFIndex idx)
{
	const struct __CFString *s = (const struct __CFString *)theString;
	CFIndex off = charIndexToByteOffset(s, idx);
	if (off >= s->byteLength)
		return 0;
	int consumed;
	uint32_t cp = utf8Decode(s->buffer + off, &consumed);
	return (UniChar)cp;	/* truncates non-BMP codepoints -- documented */
}

void CFStringGetCharacters(CFStringRef theString, CFRange range, UniChar *buffer)
{
	for (CFIndex i = 0; i < range.length; i++)
		buffer[i] = CFStringGetCharacterAtIndex(theString, range.location + i);
}

Boolean CFStringGetCString(CFStringRef theString, char *buffer, CFIndex bufferSize, CFStringEncoding encoding)
{
	const struct __CFString *s = (const struct __CFString *)theString;
	if (encoding == kCFStringEncodingASCII) {
		for (CFIndex i = 0; i < s->byteLength; i++)
			if ((unsigned char)s->buffer[i] >= 0x80)
				return false;
	}
	if (s->byteLength + 1 > bufferSize)
		return false;
	memcpy(buffer, s->buffer, (size_t)s->byteLength);
	buffer[s->byteLength] = '\0';
	return true;
}

const char *CFStringGetCStringPtr(CFStringRef theString, CFStringEncoding encoding)
{
	const struct __CFString *s = (const struct __CFString *)theString;
	if (encoding == kCFStringEncodingUTF8)
		return s->buffer;
	if (encoding == kCFStringEncodingASCII) {
		for (CFIndex i = 0; i < s->byteLength; i++)
			if ((unsigned char)s->buffer[i] >= 0x80)
				return NULL;
		return s->buffer;
	}
	return NULL;
}

CFIndex CFStringGetMaximumSizeForEncoding(CFIndex length, CFStringEncoding encoding)
{
	if (encoding == kCFStringEncodingASCII || encoding == kCFStringEncodingMacRoman)
		return length;
	return length * 4;	/* worst case for UTF-8 */
}

/* ---- comparison / search ---- */

CFComparisonResult CFStringCompare(CFStringRef theString1, CFStringRef theString2, CFStringCompareFlags compareOptions)
{
	const struct __CFString *a = (const struct __CFString *)theString1;
	const struct __CFString *b = (const struct __CFString *)theString2;
	int r;
	if (compareOptions & kCFCompareCaseInsensitive) {
#if defined(__APPLE__) || 1
		r = strcasecmp(a->buffer, b->buffer);
#endif
	} else {
		r = strcmp(a->buffer, b->buffer);
	}
	return r < 0 ? kCFCompareLessThan : (r > 0 ? kCFCompareGreaterThan : kCFCompareEqualTo);
}

Boolean CFStringHasPrefix(CFStringRef theString, CFStringRef prefix)
{
	const struct __CFString *a = (const struct __CFString *)theString;
	const struct __CFString *b = (const struct __CFString *)prefix;
	if (b->byteLength > a->byteLength)
		return false;
	return memcmp(a->buffer, b->buffer, (size_t)b->byteLength) == 0;
}

Boolean CFStringHasSuffix(CFStringRef theString, CFStringRef suffix)
{
	const struct __CFString *a = (const struct __CFString *)theString;
	const struct __CFString *b = (const struct __CFString *)suffix;
	if (b->byteLength > a->byteLength)
		return false;
	return memcmp(a->buffer + (a->byteLength - b->byteLength), b->buffer, (size_t)b->byteLength) == 0;
}

Boolean CFStringFind(CFStringRef theString, CFStringRef stringToFind, CFStringCompareFlags compareOptions, CFRange *result)
{
	(void)compareOptions;	/* v1: plain byte search only, no case-insensitive/backwards find */
	const struct __CFString *a = (const struct __CFString *)theString;
	const struct __CFString *b = (const struct __CFString *)stringToFind;
	if (b->byteLength == 0 || b->byteLength > a->byteLength)
		return false;
	for (CFIndex off = 0; off + b->byteLength <= a->byteLength; off++) {
		if (memcmp(a->buffer + off, b->buffer, (size_t)b->byteLength) != 0)
			continue;
		if (result) {
			/* translate byte offsets back to character indices */
			CFIndex startChar = 0, o = 0;
			while (o < off) {
				int consumed;
				utf8Decode(a->buffer + o, &consumed);
				o += consumed;
				startChar++;
			}
			CFIndex lenChar = 0;
			while (o < off + b->byteLength) {
				int consumed;
				utf8Decode(a->buffer + o, &consumed);
				o += consumed;
				lenChar++;
			}
			*result = CFRangeMake(startChar, lenChar);
		}
		return true;
	}
	return false;
}

/* ---- more mutation ---- */

void CFStringAppendFormat(CFMutableStringRef theString, CFTypeRef formatOptions, CFStringRef format, ...)
{
	va_list args;
	va_start(args, format);
	CFStringRef piece = CFStringCreateWithFormatAndArguments(kCFAllocatorDefault, formatOptions, format, args);
	va_end(args);
	CFStringAppend(theString, piece);
	CFRelease(piece);
}

void CFStringInsert(CFMutableStringRef str, CFIndex idx, CFStringRef insertedStr)
{
	struct __CFString *s = (struct __CFString *)str;
	const struct __CFString *ins = (const struct __CFString *)insertedStr;
	CFIndex off = charIndexToByteOffset(s, idx);
	CFIndex needed = s->byteLength + ins->byteLength + 1;
	if (needed > s->capacity) {
		while (s->capacity < needed)
			s->capacity *= 2;
		s->buffer = realloc(s->buffer, (size_t)s->capacity);
	}
	memmove(s->buffer + off + ins->byteLength, s->buffer + off, (size_t)(s->byteLength - off));
	memcpy(s->buffer + off, ins->buffer, (size_t)ins->byteLength);
	s->byteLength += ins->byteLength;
	s->buffer[s->byteLength] = '\0';
}

void CFStringDelete(CFMutableStringRef theString, CFRange range)
{
	struct __CFString *s = (struct __CFString *)theString;
	CFIndex startOff = charIndexToByteOffset(s, range.location);
	CFIndex endOff = charIndexToByteOffset(s, range.location + range.length);
	memmove(s->buffer + startOff, s->buffer + endOff, (size_t)(s->byteLength - endOff));
	s->byteLength -= (endOff - startOff);
	s->buffer[s->byteLength] = '\0';
}

void CFStringPad(CFMutableStringRef theString, CFStringRef padString, CFIndex length, CFIndex indexIntoPad)
{
	struct __CFString *s = (struct __CFString *)theString;
	CFIndex curLen = CFStringGetLength((CFStringRef)s);
	if (length < curLen) {
		CFStringDelete(theString, CFRangeMake(length, curLen - length));
		return;
	}
	const struct __CFString *pad = (const struct __CFString *)padString;
	CFIndex padLen = CFStringGetLength(padString);
	for (CFIndex i = curLen; i < length; i++) {
		CFIndex padIdx = (indexIntoPad + (i - curLen)) % (padLen ? padLen : 1);
		CFIndex off = charIndexToByteOffset(pad, padIdx);
		int consumed;
		utf8Decode(pad->buffer + off, &consumed);
		stringAppendBytes(s, pad->buffer + off, consumed);
	}
}

CFArrayRef CFStringCreateArrayBySeparatingStrings(CFAllocatorRef alloc, CFStringRef theString, CFStringRef separatorString)
{
	const struct __CFString *s = (const struct __CFString *)theString;
	const struct __CFString *sep = (const struct __CFString *)separatorString;
	CFMutableArrayRef arr = CFArrayCreateMutable(alloc, 0, &kCFTypeArrayCallBacks);
	if (sep->byteLength == 0 || sep->byteLength > s->byteLength) {
		CFArrayAppendValue(arr, theString);
		return arr;
	}
	CFIndex start = 0;
	for (CFIndex off = 0; off + sep->byteLength <= s->byteLength; ) {
		if (memcmp(s->buffer + off, sep->buffer, (size_t)sep->byteLength) == 0) {
			CFStringRef piece = CFStringCreateWithBytes(alloc, (const UInt8 *)s->buffer + start, off - start, kCFStringEncodingUTF8, false);
			CFArrayAppendValue(arr, piece);
			CFRelease(piece);
			off += sep->byteLength;
			start = off;
		} else {
			off++;
		}
	}
	CFStringRef last = CFStringCreateWithBytes(alloc, (const UInt8 *)s->buffer + start, s->byteLength - start, kCFStringEncodingUTF8, false);
	CFArrayAppendValue(arr, last);
	CFRelease(last);
	return arr;
}

CFStringRef CFStringCreateByCombiningStrings(CFAllocatorRef alloc, CFArrayRef theArray, CFStringRef separatorString)
{
	CFMutableStringRef result = CFStringCreateMutable(alloc, 0);
	CFIndex count = CFArrayGetCount(theArray);
	for (CFIndex i = 0; i < count; i++) {
		if (i > 0)
			CFStringAppend(result, separatorString);
		CFStringAppend(result, (CFStringRef)CFArrayGetValueAtIndex(theArray, i));
	}
	return (CFStringRef)result;
}

/* ---- CFStringCreateWithFormat ----
 *
 * Reassembles each "%..." conversion into a standalone mini format
 * string and hands it to the real vsnprintf, which both formats it and
 * (because va_list decays to a pointer the callee mutates in place on
 * this target's x86_64 SysV ABI) advances `args` by exactly the right
 * amount for that conversion's argument -- the same trick real-world
 * custom formatters use to ride on top of a libc vsnprintf without
 * reimplementing printf's type-dispatch. %@ is the one CF-specific
 * addition, handled directly via CFCopyDescription.
 *
 * Whatever userland/libc/src/stdio.c's vsnprintf supports, this
 * supports -- no more. In particular that vsnprintf has no floating-
 * point conversions at all (%f/%e/%g fall through to its `default:`
 * case, which prints the literal character and silently does NOT
 * consume the va_arg), so a %f here would desync every argument after
 * it. This is a pre-existing libc gap this file inherits rather than
 * works around; %d/%i/%u/%x/%X/%o/%c/%s/%p and their length-modified
 * forms all work correctly.
 */
CFStringRef CFStringCreateWithFormat(CFAllocatorRef alloc, CFTypeRef formatOptions, CFStringRef format, ...)
{
	va_list args;
	va_start(args, format);
	CFStringRef result = CFStringCreateWithFormatAndArguments(alloc, formatOptions, format, args);
	va_end(args);
	return result;
}

CFStringRef CFStringCreateWithFormatAndArguments(CFAllocatorRef alloc, CFTypeRef formatOptions, CFStringRef format, va_list args)
{
	(void)formatOptions;
	const struct __CFString *fmt = (const struct __CFString *)format;
	CFMutableStringRef result = CFStringCreateMutable(alloc, 0);

	const char *p = fmt->buffer;
	while (*p) {
		if (*p != '%') {
			const char *litStart = p;
			while (*p && *p != '%')
				p++;
			stringAppendBytes((struct __CFString *)result, litStart, p - litStart);
			continue;
		}

		const char *specStart = p;
		p++;	/* skip '%' */
		while (*p && strchr("-+ #0", *p))
			p++;
		while (*p && isdigit((unsigned char)*p))
			p++;
		if (*p == '.') {
			p++;
			while (*p && isdigit((unsigned char)*p))
				p++;
		}
		while (*p && strchr("lhzjtL", *p))
			p++;
		char conv = *p;
		if (conv)
			p++;

		if (conv == '%') {
			stringAppendBytes((struct __CFString *)result, "%", 1);
			continue;
		}
		if (conv == '@') {
			CFTypeRef arg = va_arg(args, CFTypeRef);
			CFStringRef desc = arg ? CFCopyDescription(arg) : CFStringCreateWithCString(kCFAllocatorDefault, "(null)", kCFStringEncodingUTF8);
			CFStringAppend(result, desc);
			CFRelease(desc);
			continue;
		}
		if (conv == '\0')
			break;

		char miniFmt[32];
		size_t specLen = (size_t)(p - specStart);
		if (specLen >= sizeof(miniFmt))
			specLen = sizeof(miniFmt) - 1;
		memcpy(miniFmt, specStart, specLen);
		miniFmt[specLen] = '\0';

		char tmp[512];
		vsnprintf(tmp, sizeof(tmp), miniFmt, args);
		stringAppendBytes((struct __CFString *)result, tmp, (CFIndex)strlen(tmp));
	}

	return (CFStringRef)result;
}
