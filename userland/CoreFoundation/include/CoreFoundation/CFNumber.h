/* Copyright (c) 2026 Vihaan Nathan
 *
 * Canonical storage is either int64_t or double (a `Boolean isFloat`
 * flag picks which); CFNumberGetValue() converts on the way out. Real
 * CF's kCFNumberPositiveInfinity/NegativeInfinity/NaN singletons and
 * CFNumberGetType() edge cases around numeric overflow are not
 * implemented -- documented v1 gap, not a silent one.
 */
#ifndef __COREFOUNDATION_CFNUMBER_H__
#define __COREFOUNDATION_CFNUMBER_H__

#include <CoreFoundation/CFBase.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef const struct __CFNumber *CFNumberRef;

typedef CFIndex CFNumberType;
enum {
	kCFNumberSInt8Type = 1,
	kCFNumberSInt16Type = 2,
	kCFNumberSInt32Type = 3,
	kCFNumberSInt64Type = 4,
	kCFNumberFloat32Type = 5,
	kCFNumberFloat64Type = 6,
	kCFNumberCharType = 7,
	kCFNumberShortType = 8,
	kCFNumberIntType = 9,
	kCFNumberLongType = 10,
	kCFNumberLongLongType = 11,
	kCFNumberFloatType = 12,
	kCFNumberDoubleType = 13,
	kCFNumberCFIndexType = 14,
	kCFNumberNSIntegerType = 15,
	kCFNumberCGFloatType = 16,
	kCFNumberMaxType = 16
};

CF_EXPORT CFTypeID CFNumberGetTypeID(void);
CF_EXPORT CFNumberRef CFNumberCreate(CFAllocatorRef allocator, CFNumberType theType, const void *valuePtr);
CF_EXPORT Boolean CFNumberGetValue(CFNumberRef number, CFNumberType theType, void *valuePtr);
CF_EXPORT CFNumberType CFNumberGetType(CFNumberRef number);
CF_EXPORT Boolean CFNumberIsFloatType(CFNumberRef number);
CF_EXPORT CFComparisonResult CFNumberCompare(CFNumberRef number, CFNumberRef otherNumber, void *context);

/* ---- CFBoolean ---- */
typedef const struct __CFBoolean *CFBooleanRef;
CF_EXPORT const CFBooleanRef kCFBooleanTrue;
CF_EXPORT const CFBooleanRef kCFBooleanFalse;
CF_EXPORT CFTypeID CFBooleanGetTypeID(void);
CF_EXPORT Boolean CFBooleanGetValue(CFBooleanRef boolean);

#ifdef __cplusplus
}
#endif

#endif /* __COREFOUNDATION_CFNUMBER_H__ */
