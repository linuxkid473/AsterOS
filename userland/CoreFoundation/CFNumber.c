/* Copyright (c) 2026 Vihaan Nathan
 *
 * Canonical storage is int64_t or double (see CFNumber.h). CFNumberCreate
 * reads valuePtr according to theType's real on-the-wire size/signedness,
 * CFNumberGetValue writes back with a narrowing conversion -- same
 * contract as real CF, minus the exact-overflow-detection return value
 * (this always reports success; real CF can report false on truncation).
 */
#include "CFInternal.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>

struct __CFNumber {
	CFRuntimeBase base;
	Boolean isFloat;
	union {
		int64_t i;
		double d;
	} v;
};

static CFTypeID g_numberTypeID;
static pthread_once_t g_numberOnce = PTHREAD_ONCE_INIT;

static Boolean numberEqual(CFTypeRef cf1, CFTypeRef cf2)
{
	const struct __CFNumber *a = cf1, *b = cf2;
	double av = a->isFloat ? a->v.d : (double)a->v.i;
	double bv = b->isFloat ? b->v.d : (double)b->v.i;
	return av == bv;
}

static CFHashCode numberHash(CFTypeRef cf)
{
	const struct __CFNumber *n = cf;
	return n->isFloat ? (CFHashCode)n->v.d : (CFHashCode)n->v.i;
}

static CFStringRef numberCopyDesc(CFTypeRef cf)
{
	const struct __CFNumber *n = cf;
	char buf[64];
	if (n->isFloat)
		snprintf(buf, sizeof(buf), "%g", n->v.d);
	else
		snprintf(buf, sizeof(buf), "%lld", (long long)n->v.i);
	return CFStringCreateWithCString(kCFAllocatorDefault, buf, kCFStringEncodingUTF8);
}

static void numberInit(void)
{
	static const CFRuntimeClass cls = {
		.className = "CFNumber",
		.equal = numberEqual,
		.hash = numberHash,
		.copyFormattingDesc = numberCopyDesc,
	};
	g_numberTypeID = _CFRuntimeRegisterClass(&cls);
}

CFTypeID CFNumberGetTypeID(void)
{
	pthread_once(&g_numberOnce, numberInit);
	return g_numberTypeID;
}

static Boolean typeIsFloat(CFNumberType t)
{
	return t == kCFNumberFloat32Type || t == kCFNumberFloat64Type || t == kCFNumberFloatType || t == kCFNumberDoubleType || t == kCFNumberCGFloatType;
}

CFNumberRef CFNumberCreate(CFAllocatorRef allocator, CFNumberType theType, const void *valuePtr)
{
	CFNumberGetTypeID();
	struct __CFNumber *n = (struct __CFNumber *)_CFRuntimeCreateInstance(allocator, g_numberTypeID, sizeof(struct __CFNumber) - sizeof(CFRuntimeBase));
	n->isFloat = typeIsFloat(theType);
	switch (theType) {
	case kCFNumberSInt8Type: n->v.i = *(const int8_t *)valuePtr; break;
	case kCFNumberSInt16Type: case kCFNumberShortType: n->v.i = *(const int16_t *)valuePtr; break;
	case kCFNumberSInt32Type: case kCFNumberIntType: case kCFNumberCFIndexType: n->v.i = *(const int32_t *)valuePtr; break;
	case kCFNumberSInt64Type: case kCFNumberLongType: case kCFNumberLongLongType: case kCFNumberNSIntegerType: n->v.i = *(const int64_t *)valuePtr; break;
	case kCFNumberCharType: n->v.i = *(const char *)valuePtr; break;
	case kCFNumberFloat32Type: case kCFNumberFloatType: n->v.d = *(const float *)valuePtr; break;
	case kCFNumberFloat64Type: case kCFNumberDoubleType: case kCFNumberCGFloatType: n->v.d = *(const double *)valuePtr; break;
	default: n->v.i = 0; break;
	}
	return (CFNumberRef)n;
}

Boolean CFNumberGetValue(CFNumberRef number, CFNumberType theType, void *valuePtr)
{
	const struct __CFNumber *n = number;
	double dv = n->isFloat ? n->v.d : (double)n->v.i;
	int64_t iv = n->isFloat ? (int64_t)n->v.d : n->v.i;
	switch (theType) {
	case kCFNumberSInt8Type: *(int8_t *)valuePtr = (int8_t)iv; break;
	case kCFNumberSInt16Type: case kCFNumberShortType: *(int16_t *)valuePtr = (int16_t)iv; break;
	case kCFNumberSInt32Type: case kCFNumberIntType: case kCFNumberCFIndexType: *(int32_t *)valuePtr = (int32_t)iv; break;
	case kCFNumberSInt64Type: case kCFNumberLongType: case kCFNumberLongLongType: case kCFNumberNSIntegerType: *(int64_t *)valuePtr = iv; break;
	case kCFNumberCharType: *(char *)valuePtr = (char)iv; break;
	case kCFNumberFloat32Type: case kCFNumberFloatType: *(float *)valuePtr = (float)dv; break;
	case kCFNumberFloat64Type: case kCFNumberDoubleType: case kCFNumberCGFloatType: *(double *)valuePtr = dv; break;
	default: return false;
	}
	return true;
}

CFNumberType CFNumberGetType(CFNumberRef number)
{
	return number->isFloat ? kCFNumberFloat64Type : kCFNumberSInt64Type;
}

Boolean CFNumberIsFloatType(CFNumberRef number)
{
	return number->isFloat;
}

CFComparisonResult CFNumberCompare(CFNumberRef number, CFNumberRef otherNumber, void *context)
{
	(void)context;
	double a = number->isFloat ? number->v.d : (double)number->v.i;
	double b = otherNumber->isFloat ? otherNumber->v.d : (double)otherNumber->v.i;
	if (a < b)
		return kCFCompareLessThan;
	if (a > b)
		return kCFCompareGreaterThan;
	return kCFCompareEqualTo;
}
