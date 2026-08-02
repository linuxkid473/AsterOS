/* Copyright (c) 2026 Vihaan Nathan */
#include "CFInternal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

struct __CFArray {
	CFRuntimeBase base;
	const void **values;
	CFIndex count;
	CFIndex capacity;
	CFArrayCallBacks callbacks;
	Boolean hasCallbacks;
};

static CFTypeID g_arrayTypeID;
static pthread_once_t g_arrayOnce = PTHREAD_ONCE_INIT;

static const void *typeRetain(CFAllocatorRef allocator, const void *value)
{
	(void)allocator;
	return CFRetain(value);
}

static void typeRelease(CFAllocatorRef allocator, const void *value)
{
	(void)allocator;
	CFRelease(value);
}

static CFStringRef typeCopyDescription(const void *value)
{
	return CFCopyDescription(value);
}

static Boolean typeEqual(const void *value1, const void *value2)
{
	return CFEqual(value1, value2);
}

const CFArrayCallBacks kCFTypeArrayCallBacks = {
	.version = 0,
	.retain = typeRetain,
	.release = typeRelease,
	.copyDescription = typeCopyDescription,
	.equal = typeEqual,
};

static void arrayFinalize(CFTypeRef cf)
{
	struct __CFArray *a = (struct __CFArray *)cf;
	if (a->hasCallbacks && a->callbacks.release)
		for (CFIndex i = 0; i < a->count; i++)
			a->callbacks.release(kCFAllocatorDefault, a->values[i]);
	free(a->values);
}

static Boolean arrayEqual(CFTypeRef cf1, CFTypeRef cf2)
{
	const struct __CFArray *a = cf1, *b = cf2;
	if (a->count != b->count)
		return false;
	for (CFIndex i = 0; i < a->count; i++) {
		Boolean eq = (a->hasCallbacks && a->callbacks.equal) ? a->callbacks.equal(a->values[i], b->values[i]) : (a->values[i] == b->values[i]);
		if (!eq)
			return false;
	}
	return true;
}

static CFHashCode arrayHash(CFTypeRef cf)
{
	const struct __CFArray *a = cf;
	return (CFHashCode)a->count;
}

static CFStringRef arrayCopyDesc(CFTypeRef cf)
{
	const struct __CFArray *a = cf;
	CFMutableStringRef desc = CFStringCreateMutable(kCFAllocatorDefault, 0);
	CFStringAppendCString(desc, "(", kCFStringEncodingUTF8);
	for (CFIndex i = 0; i < a->count; i++) {
		if (i > 0)
			CFStringAppendCString(desc, ", ", kCFStringEncodingUTF8);
		if (a->hasCallbacks && a->callbacks.copyDescription) {
			CFStringRef elemDesc = a->callbacks.copyDescription(a->values[i]);
			CFStringAppend(desc, elemDesc);
			CFRelease(elemDesc);
		} else {
			char buf[32];
			snprintf(buf, sizeof(buf), "%p", a->values[i]);
			CFStringAppendCString(desc, buf, kCFStringEncodingUTF8);
		}
	}
	CFStringAppendCString(desc, ")", kCFStringEncodingUTF8);
	return (CFStringRef)desc;
}

static void arrayInit(void)
{
	static const CFRuntimeClass cls = {
		.className = "CFArray",
		.finalize = arrayFinalize,
		.equal = arrayEqual,
		.hash = arrayHash,
		.copyFormattingDesc = arrayCopyDesc,
	};
	g_arrayTypeID = _CFRuntimeRegisterClass(&cls);
}

CFTypeID CFArrayGetTypeID(void)
{
	pthread_once(&g_arrayOnce, arrayInit);
	return g_arrayTypeID;
}

static struct __CFArray *arrayAlloc(CFAllocatorRef allocator, CFIndex capacity, const CFArrayCallBacks *callBacks, Boolean mutableFlag)
{
	(void)mutableFlag;
	CFArrayGetTypeID();
	struct __CFArray *a = (struct __CFArray *)_CFRuntimeCreateInstance(allocator, g_arrayTypeID, sizeof(struct __CFArray) - sizeof(CFRuntimeBase));
	if (capacity < 4)
		capacity = 4;
	a->capacity = capacity;
	a->values = malloc(sizeof(const void *) * (size_t)a->capacity);
	a->count = 0;
	if (callBacks) {
		a->callbacks = *callBacks;
		a->hasCallbacks = true;
	}
	return a;
}

CFArrayRef CFArrayCreate(CFAllocatorRef allocator, const void **values, CFIndex numValues, const CFArrayCallBacks *callBacks)
{
	struct __CFArray *a = arrayAlloc(allocator, numValues, callBacks, false);
	for (CFIndex i = 0; i < numValues; i++)
		a->values[i] = (a->hasCallbacks && a->callbacks.retain) ? a->callbacks.retain(allocator, values[i]) : values[i];
	a->count = numValues;
	return (CFArrayRef)a;
}

CFArrayRef CFArrayCreateCopy(CFAllocatorRef allocator, CFArrayRef theArray)
{
	const struct __CFArray *src = (const struct __CFArray *)theArray;
	return CFArrayCreate(allocator, src->values, src->count, src->hasCallbacks ? &src->callbacks : NULL);
}

CFMutableArrayRef CFArrayCreateMutable(CFAllocatorRef allocator, CFIndex capacity, const CFArrayCallBacks *callBacks)
{
	return (CFMutableArrayRef)arrayAlloc(allocator, capacity, callBacks, true);
}

CFMutableArrayRef CFArrayCreateMutableCopy(CFAllocatorRef allocator, CFIndex capacity, CFArrayRef theArray)
{
	const struct __CFArray *src = (const struct __CFArray *)theArray;
	struct __CFArray *a = arrayAlloc(allocator, capacity > src->count ? capacity : src->count, src->hasCallbacks ? &src->callbacks : NULL, true);
	for (CFIndex i = 0; i < src->count; i++)
		a->values[i] = (a->hasCallbacks && a->callbacks.retain) ? a->callbacks.retain(allocator, src->values[i]) : src->values[i];
	a->count = src->count;
	return (CFMutableArrayRef)a;
}

CFIndex CFArrayGetCount(CFArrayRef theArray)
{
	return ((const struct __CFArray *)theArray)->count;
}

const void *CFArrayGetValueAtIndex(CFArrayRef theArray, CFIndex idx)
{
	return ((const struct __CFArray *)theArray)->values[idx];
}

void CFArrayGetValues(CFArrayRef theArray, CFRange range, const void **values)
{
	const struct __CFArray *a = (const struct __CFArray *)theArray;
	memcpy(values, a->values + range.location, sizeof(const void *) * (size_t)range.length);
}

void CFArrayApplyFunction(CFArrayRef theArray, CFRange range, CFArrayApplierFunction applier, void *context)
{
	const struct __CFArray *a = (const struct __CFArray *)theArray;
	for (CFIndex i = 0; i < range.length; i++)
		applier(a->values[range.location + i], context);
}

Boolean CFArrayContainsValue(CFArrayRef theArray, CFRange range, const void *value)
{
	return CFArrayGetFirstIndexOfValue(theArray, range, value) != -1;
}

CFIndex CFArrayGetFirstIndexOfValue(CFArrayRef theArray, CFRange range, const void *value)
{
	const struct __CFArray *a = (const struct __CFArray *)theArray;
	for (CFIndex i = 0; i < range.length; i++) {
		CFIndex idx = range.location + i;
		Boolean eq = (a->hasCallbacks && a->callbacks.equal) ? a->callbacks.equal(a->values[idx], value) : (a->values[idx] == value);
		if (eq)
			return idx;
	}
	return -1;
}

static void arrayEnsureCapacity(struct __CFArray *a, CFIndex needed)
{
	if (needed <= a->capacity)
		return;
	while (a->capacity < needed)
		a->capacity *= 2;
	a->values = realloc(a->values, sizeof(const void *) * (size_t)a->capacity);
}

void CFArrayAppendValue(CFMutableArrayRef theArray, const void *value)
{
	struct __CFArray *a = (struct __CFArray *)theArray;
	arrayEnsureCapacity(a, a->count + 1);
	a->values[a->count++] = (a->hasCallbacks && a->callbacks.retain) ? a->callbacks.retain(kCFAllocatorDefault, value) : value;
}

void CFArrayInsertValueAtIndex(CFMutableArrayRef theArray, CFIndex idx, const void *value)
{
	struct __CFArray *a = (struct __CFArray *)theArray;
	arrayEnsureCapacity(a, a->count + 1);
	memmove(a->values + idx + 1, a->values + idx, sizeof(const void *) * (size_t)(a->count - idx));
	a->values[idx] = (a->hasCallbacks && a->callbacks.retain) ? a->callbacks.retain(kCFAllocatorDefault, value) : value;
	a->count++;
}

void CFArraySetValueAtIndex(CFMutableArrayRef theArray, CFIndex idx, const void *value)
{
	struct __CFArray *a = (struct __CFArray *)theArray;
	if (a->hasCallbacks && a->callbacks.release)
		a->callbacks.release(kCFAllocatorDefault, a->values[idx]);
	a->values[idx] = (a->hasCallbacks && a->callbacks.retain) ? a->callbacks.retain(kCFAllocatorDefault, value) : value;
}

void CFArrayRemoveValueAtIndex(CFMutableArrayRef theArray, CFIndex idx)
{
	struct __CFArray *a = (struct __CFArray *)theArray;
	if (a->hasCallbacks && a->callbacks.release)
		a->callbacks.release(kCFAllocatorDefault, a->values[idx]);
	memmove(a->values + idx, a->values + idx + 1, sizeof(const void *) * (size_t)(a->count - idx - 1));
	a->count--;
}

void CFArrayRemoveAllValues(CFMutableArrayRef theArray)
{
	struct __CFArray *a = (struct __CFArray *)theArray;
	if (a->hasCallbacks && a->callbacks.release)
		for (CFIndex i = 0; i < a->count; i++)
			a->callbacks.release(kCFAllocatorDefault, a->values[i]);
	a->count = 0;
}

void CFArrayAppendArray(CFMutableArrayRef theArray, CFArrayRef otherArray, CFRange otherRange)
{
	const struct __CFArray *o = (const struct __CFArray *)otherArray;
	for (CFIndex i = 0; i < otherRange.length; i++)
		CFArrayAppendValue(theArray, o->values[otherRange.location + i]);
}

void CFArraySortValues(CFMutableArrayRef theArray, CFRange range, CFComparatorFunction comparator, void *context)
{
	struct __CFArray *a = (struct __CFArray *)theArray;
	/* insertion sort -- arrays in this OS's actual use (test/tooling
	 * data, not bulk datasets) are small enough that O(n^2) is fine and
	 * this avoids pulling qsort_r-style context-passing into libc. */
	for (CFIndex i = range.location + 1; i < range.location + range.length; i++) {
		const void *key = a->values[i];
		CFIndex j = i - 1;
		while (j >= range.location && comparator(a->values[j], key, context) == kCFCompareGreaterThan) {
			a->values[j + 1] = a->values[j];
			j--;
		}
		a->values[j + 1] = key;
	}
}
