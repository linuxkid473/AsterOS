/* Copyright (c) 2026 Vihaan Nathan */
#include "CFInternal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

struct __CFData {
	CFRuntimeBase base;
	UInt8 *bytes;
	CFIndex length;
	CFIndex capacity;
	Boolean isMutable;
};

static CFTypeID g_dataTypeID;
static pthread_once_t g_dataOnce = PTHREAD_ONCE_INIT;

static void dataFinalize(CFTypeRef cf)
{
	free(((struct __CFData *)cf)->bytes);
}

static Boolean dataEqual(CFTypeRef cf1, CFTypeRef cf2)
{
	const struct __CFData *a = cf1, *b = cf2;
	return a->length == b->length && memcmp(a->bytes, b->bytes, (size_t)a->length) == 0;
}

static CFHashCode dataHash(CFTypeRef cf)
{
	const struct __CFData *d = cf;
	CFHashCode h = 2166136261u;
	for (CFIndex i = 0; i < d->length; i++) {
		h ^= d->bytes[i];
		h *= 16777619u;
	}
	return h;
}

static CFStringRef dataCopyDesc(CFTypeRef cf)
{
	const struct __CFData *d = cf;
	CFMutableStringRef s = CFStringCreateMutable(kCFAllocatorDefault, 0);
	CFStringAppendCString(s, "<", kCFStringEncodingUTF8);
	for (CFIndex i = 0; i < d->length; i++) {
		char buf[4];
		snprintf(buf, sizeof(buf), "%02x", d->bytes[i]);
		CFStringAppendCString(s, buf, kCFStringEncodingUTF8);
	}
	CFStringAppendCString(s, ">", kCFStringEncodingUTF8);
	return (CFStringRef)s;
}

static void dataInit(void)
{
	static const CFRuntimeClass cls = {
		.className = "CFData",
		.finalize = dataFinalize,
		.equal = dataEqual,
		.hash = dataHash,
		.copyFormattingDesc = dataCopyDesc,
	};
	g_dataTypeID = _CFRuntimeRegisterClass(&cls);
}

CFTypeID CFDataGetTypeID(void)
{
	pthread_once(&g_dataOnce, dataInit);
	return g_dataTypeID;
}

static struct __CFData *dataAlloc(CFAllocatorRef allocator, CFIndex capacity)
{
	CFDataGetTypeID();
	struct __CFData *d = (struct __CFData *)_CFRuntimeCreateInstance(allocator, g_dataTypeID, sizeof(struct __CFData) - sizeof(CFRuntimeBase));
	if (capacity < 1)
		capacity = 1;
	d->capacity = capacity;
	d->bytes = malloc((size_t)capacity);
	return d;
}

CFDataRef CFDataCreate(CFAllocatorRef allocator, const UInt8 *bytes, CFIndex length)
{
	struct __CFData *d = dataAlloc(allocator, length);
	memcpy(d->bytes, bytes, (size_t)length);
	d->length = length;
	return (CFDataRef)d;
}

CFDataRef CFDataCreateCopy(CFAllocatorRef allocator, CFDataRef theData)
{
	const struct __CFData *src = (const struct __CFData *)theData;
	return CFDataCreate(allocator, src->bytes, src->length);
}

CFMutableDataRef CFDataCreateMutable(CFAllocatorRef allocator, CFIndex capacity)
{
	struct __CFData *d = dataAlloc(allocator, capacity ? capacity : 16);
	d->isMutable = true;
	return (CFMutableDataRef)d;
}

CFMutableDataRef CFDataCreateMutableCopy(CFAllocatorRef allocator, CFIndex capacity, CFDataRef theData)
{
	const struct __CFData *src = (const struct __CFData *)theData;
	CFMutableDataRef m = CFDataCreateMutable(allocator, capacity > src->length ? capacity : src->length);
	CFDataAppendBytes(m, src->bytes, src->length);
	return m;
}

CFIndex CFDataGetLength(CFDataRef theData)
{
	return ((const struct __CFData *)theData)->length;
}

const UInt8 *CFDataGetBytePtr(CFDataRef theData)
{
	return ((const struct __CFData *)theData)->bytes;
}

UInt8 *CFDataGetMutableBytePtr(CFMutableDataRef theData)
{
	return ((struct __CFData *)theData)->bytes;
}

void CFDataGetBytes(CFDataRef theData, CFRange range, UInt8 *buffer)
{
	const struct __CFData *d = (const struct __CFData *)theData;
	memcpy(buffer, d->bytes + range.location, (size_t)range.length);
}

static void dataEnsureCapacity(struct __CFData *d, CFIndex needed)
{
	if (needed <= d->capacity)
		return;
	while (d->capacity < needed)
		d->capacity *= 2;
	d->bytes = realloc(d->bytes, (size_t)d->capacity);
}

void CFDataAppendBytes(CFMutableDataRef theData, const UInt8 *bytes, CFIndex length)
{
	struct __CFData *d = (struct __CFData *)theData;
	dataEnsureCapacity(d, d->length + length);
	memcpy(d->bytes + d->length, bytes, (size_t)length);
	d->length += length;
}

void CFDataDeleteBytes(CFMutableDataRef theData, CFRange range)
{
	struct __CFData *d = (struct __CFData *)theData;
	memmove(d->bytes + range.location, d->bytes + range.location + range.length, (size_t)(d->length - range.location - range.length));
	d->length -= range.length;
}

void CFDataReplaceBytes(CFMutableDataRef theData, CFRange range, const UInt8 *newBytes, CFIndex newLength)
{
	struct __CFData *d = (struct __CFData *)theData;
	CFIndex tailLen = d->length - range.location - range.length;
	CFIndex delta = newLength - range.length;
	dataEnsureCapacity(d, d->length + (delta > 0 ? delta : 0));
	memmove(d->bytes + range.location + newLength, d->bytes + range.location + range.length, (size_t)tailLen);
	memcpy(d->bytes + range.location, newBytes, (size_t)newLength);
	d->length += delta;
}

void CFDataSetLength(CFMutableDataRef theData, CFIndex length)
{
	struct __CFData *d = (struct __CFData *)theData;
	dataEnsureCapacity(d, length);
	if (length > d->length)
		memset(d->bytes + d->length, 0, (size_t)(length - d->length));
	d->length = length;
}

void CFDataIncreaseLength(CFMutableDataRef theData, CFIndex extraLength)
{
	struct __CFData *d = (struct __CFData *)theData;
	CFDataSetLength(theData, d->length + extraLength);
}
