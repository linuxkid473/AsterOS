/* Copyright (c) 2026 Vihaan Nathan
 *
 * v1 simplification: strings are stored as UTF-8 internally rather than
 * real CF's UTF-16 UniChar buffers. CFStringGetLength()/
 * CFStringGetCharacterAtIndex() still hand back UTF-16 code units (they
 * decode UTF-8 on the fly), so correctly-written client code sees the
 * documented behavior -- the only real gap is codepoints outside the
 * BMP, which would need surrogate pairs this decoder doesn't produce.
 * Documented, not silent: see TODO.md's CoreFoundation phase writeup.
 */
#ifndef __COREFOUNDATION_CFSTRING_H__
#define __COREFOUNDATION_CFSTRING_H__

#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFArray.h>
#include <stdarg.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned short UniChar;
typedef uint32_t CFStringEncoding;

#define kCFStringEncodingInvalidId ((CFStringEncoding)0xffffffffU)
enum {
	kCFStringEncodingMacRoman = 0,
	kCFStringEncodingASCII = 0x0600,
	kCFStringEncodingUTF8 = 0x08000100,
	kCFStringEncodingUnicode = 0x0100,
	kCFStringEncodingUTF16 = 0x0100
};

typedef CFOptionFlags CFStringCompareFlags;
enum {
	kCFCompareCaseInsensitive = 1,
	kCFCompareBackwards = 4,
	kCFCompareAnchored = 8
};

CF_EXPORT CFTypeID CFStringGetTypeID(void);

/* ---- creation ---- */
CF_EXPORT CFStringRef CFStringCreateWithCString(CFAllocatorRef alloc, const char *cStr, CFStringEncoding encoding);
CF_EXPORT CFStringRef CFStringCreateWithBytes(CFAllocatorRef alloc, const UInt8 *bytes, CFIndex numBytes, CFStringEncoding encoding, Boolean isExternalRepresentation);
CF_EXPORT CFStringRef CFStringCreateCopy(CFAllocatorRef alloc, CFStringRef theString);
CF_EXPORT CFStringRef CFStringCreateWithFormat(CFAllocatorRef alloc, CFTypeRef formatOptions, CFStringRef format, ...);
CF_EXPORT CFStringRef CFStringCreateWithFormatAndArguments(CFAllocatorRef alloc, CFTypeRef formatOptions, CFStringRef format, va_list arguments);

CF_EXPORT CFMutableStringRef CFStringCreateMutable(CFAllocatorRef alloc, CFIndex maxLength);
CF_EXPORT CFMutableStringRef CFStringCreateMutableCopy(CFAllocatorRef alloc, CFIndex maxLength, CFStringRef theString);

/* ---- inspection ---- */
CF_EXPORT CFIndex CFStringGetLength(CFStringRef theString);
CF_EXPORT UniChar CFStringGetCharacterAtIndex(CFStringRef theString, CFIndex idx);
CF_EXPORT void CFStringGetCharacters(CFStringRef theString, CFRange range, UniChar *buffer);
CF_EXPORT Boolean CFStringGetCString(CFStringRef theString, char *buffer, CFIndex bufferSize, CFStringEncoding encoding);
CF_EXPORT const char *CFStringGetCStringPtr(CFStringRef theString, CFStringEncoding encoding);
CF_EXPORT CFIndex CFStringGetLength(CFStringRef theString);
CF_EXPORT CFIndex CFStringGetMaximumSizeForEncoding(CFIndex length, CFStringEncoding encoding);

CF_EXPORT CFComparisonResult CFStringCompare(CFStringRef theString1, CFStringRef theString2, CFStringCompareFlags compareOptions);
CF_EXPORT Boolean CFStringHasPrefix(CFStringRef theString, CFStringRef prefix);
CF_EXPORT Boolean CFStringHasSuffix(CFStringRef theString, CFStringRef suffix);
CF_EXPORT Boolean CFStringFind(CFStringRef theString, CFStringRef stringToFind, CFStringCompareFlags compareOptions, CFRange *result);

/* ---- mutation ---- */
CF_EXPORT void CFStringAppend(CFMutableStringRef theString, CFStringRef appendedString);
CF_EXPORT void CFStringAppendCString(CFMutableStringRef theString, const char *cStr, CFStringEncoding encoding);
CF_EXPORT void CFStringAppendFormat(CFMutableStringRef theString, CFTypeRef formatOptions, CFStringRef format, ...);
CF_EXPORT void CFStringInsert(CFMutableStringRef str, CFIndex idx, CFStringRef insertedStr);
CF_EXPORT void CFStringDelete(CFMutableStringRef theString, CFRange range);
CF_EXPORT void CFStringPad(CFMutableStringRef theString, CFStringRef padString, CFIndex length, CFIndex indexIntoPad);

CF_EXPORT CFArrayRef CFStringCreateArrayBySeparatingStrings(CFAllocatorRef alloc, CFStringRef theString, CFStringRef separatorString);
CF_EXPORT CFStringRef CFStringCreateByCombiningStrings(CFAllocatorRef alloc, CFArrayRef theArray, CFStringRef separatorString);

#ifdef __cplusplus
}
#endif

#endif /* __COREFOUNDATION_CFSTRING_H__ */
