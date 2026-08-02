/* Copyright (c) 2026 Vihaan Nathan
 *
 * See CFDictionary.h: linear key/value arrays, not a hash table -- O(n)
 * lookup, documented v1 tradeoff. hash callback exists for API
 * compatibility (kCFTypeDictionaryKeyCallBacks wires one up) but this
 * implementation never calls it; equal is what actually drives lookup.
 */
#include "CFInternal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

struct __CFDictionary {
	CFRuntimeBase base;
	const void **keys;
	const void **values;
	CFIndex count;
	CFIndex capacity;
	CFDictionaryKeyCallBacks keyCB;
	CFDictionaryValueCallBacks valueCB;
	Boolean hasCB;
};

static CFTypeID g_dictTypeID;
static pthread_once_t g_dictOnce = PTHREAD_ONCE_INIT;

static const void *dkRetain(CFAllocatorRef a, const void *v) { (void)a; return CFRetain(v); }
static void dkRelease(CFAllocatorRef a, const void *v) { (void)a; CFRelease(v); }
static CFStringRef dkCopyDesc(const void *v) { return CFCopyDescription(v); }
static Boolean dkEqual(const void *v1, const void *v2) { return CFEqual(v1, v2); }
static CFHashCode dkHash(const void *v) { return CFHash(v); }

const CFDictionaryKeyCallBacks kCFTypeDictionaryKeyCallBacks = {
	.version = 0, .retain = dkRetain, .release = dkRelease,
	.copyDescription = dkCopyDesc, .equal = dkEqual, .hash = dkHash,
};

const CFDictionaryValueCallBacks kCFTypeDictionaryValueCallBacks = {
	.version = 0, .retain = dkRetain, .release = dkRelease,
	.copyDescription = dkCopyDesc, .equal = dkEqual,
};

static void dictFinalize(CFTypeRef cf)
{
	struct __CFDictionary *d = (struct __CFDictionary *)cf;
	if (d->hasCB) {
		for (CFIndex i = 0; i < d->count; i++) {
			if (d->keyCB.release)
				d->keyCB.release(kCFAllocatorDefault, d->keys[i]);
			if (d->valueCB.release)
				d->valueCB.release(kCFAllocatorDefault, d->values[i]);
		}
	}
	free(d->keys);
	free(d->values);
}

static Boolean dictKeyEqual(const struct __CFDictionary *d, const void *a, const void *b)
{
	return (d->hasCB && d->keyCB.equal) ? d->keyCB.equal(a, b) : (a == b);
}

static Boolean dictEqual(CFTypeRef cf1, CFTypeRef cf2)
{
	const struct __CFDictionary *a = cf1, *b = cf2;
	if (a->count != b->count)
		return false;
	for (CFIndex i = 0; i < a->count; i++) {
		const void *v = CFDictionaryGetValue((CFDictionaryRef)b, a->keys[i]);
		if (!v)
			return false;
		Boolean eq = (a->hasCB && a->valueCB.equal) ? a->valueCB.equal(a->values[i], v) : (a->values[i] == v);
		if (!eq)
			return false;
	}
	return true;
}

static CFHashCode dictHash(CFTypeRef cf)
{
	return (CFHashCode)((const struct __CFDictionary *)cf)->count;
}

static CFStringRef dictCopyDesc(CFTypeRef cf)
{
	const struct __CFDictionary *d = cf;
	CFMutableStringRef desc = CFStringCreateMutable(kCFAllocatorDefault, 0);
	CFStringAppendCString(desc, "{", kCFStringEncodingUTF8);
	for (CFIndex i = 0; i < d->count; i++) {
		if (i > 0)
			CFStringAppendCString(desc, ", ", kCFStringEncodingUTF8);
		CFStringRef kd = (d->hasCB && d->keyCB.copyDescription) ? d->keyCB.copyDescription(d->keys[i]) : NULL;
		if (kd) {
			CFStringAppend(desc, kd);
			CFRelease(kd);
		} else {
			char kbuf[32];
			snprintf(kbuf, sizeof(kbuf), "%p", d->keys[i]);
			CFStringAppendCString(desc, kbuf, kCFStringEncodingUTF8);
		}
		CFStringAppendCString(desc, " = ", kCFStringEncodingUTF8);
		CFStringRef vd = (d->hasCB && d->valueCB.copyDescription) ? d->valueCB.copyDescription(d->values[i]) : NULL;
		if (vd) {
			CFStringAppend(desc, vd);
			CFRelease(vd);
		} else {
			char vbuf[32];
			snprintf(vbuf, sizeof(vbuf), "%p", d->values[i]);
			CFStringAppendCString(desc, vbuf, kCFStringEncodingUTF8);
		}
	}
	CFStringAppendCString(desc, "}", kCFStringEncodingUTF8);
	return (CFStringRef)desc;
}

static void dictInit(void)
{
	static const CFRuntimeClass cls = {
		.className = "CFDictionary",
		.finalize = dictFinalize,
		.equal = dictEqual,
		.hash = dictHash,
		.copyFormattingDesc = dictCopyDesc,
	};
	g_dictTypeID = _CFRuntimeRegisterClass(&cls);
}

CFTypeID CFDictionaryGetTypeID(void)
{
	pthread_once(&g_dictOnce, dictInit);
	return g_dictTypeID;
}

static struct __CFDictionary *dictAlloc(CFAllocatorRef allocator, CFIndex capacity, const CFDictionaryKeyCallBacks *keyCB, const CFDictionaryValueCallBacks *valueCB)
{
	CFDictionaryGetTypeID();
	struct __CFDictionary *d = (struct __CFDictionary *)_CFRuntimeCreateInstance(allocator, g_dictTypeID, sizeof(struct __CFDictionary) - sizeof(CFRuntimeBase));
	if (capacity < 4)
		capacity = 4;
	d->capacity = capacity;
	d->keys = malloc(sizeof(const void *) * (size_t)capacity);
	d->values = malloc(sizeof(const void *) * (size_t)capacity);
	if (keyCB || valueCB) {
		d->hasCB = true;
		if (keyCB)
			d->keyCB = *keyCB;
		if (valueCB)
			d->valueCB = *valueCB;
	}
	return d;
}

CFDictionaryRef CFDictionaryCreate(CFAllocatorRef allocator, const void **keys, const void **values, CFIndex numValues, const CFDictionaryKeyCallBacks *keyCallBacks, const CFDictionaryValueCallBacks *valueCallBacks)
{
	struct __CFDictionary *d = dictAlloc(allocator, numValues, keyCallBacks, valueCallBacks);
	for (CFIndex i = 0; i < numValues; i++) {
		d->keys[i] = (d->hasCB && d->keyCB.retain) ? d->keyCB.retain(allocator, keys[i]) : keys[i];
		d->values[i] = (d->hasCB && d->valueCB.retain) ? d->valueCB.retain(allocator, values[i]) : values[i];
	}
	d->count = numValues;
	return (CFDictionaryRef)d;
}

CFDictionaryRef CFDictionaryCreateCopy(CFAllocatorRef allocator, CFDictionaryRef theDict)
{
	const struct __CFDictionary *src = (const struct __CFDictionary *)theDict;
	return CFDictionaryCreate(allocator, src->keys, src->values, src->count, src->hasCB ? &src->keyCB : NULL, src->hasCB ? &src->valueCB : NULL);
}

CFMutableDictionaryRef CFDictionaryCreateMutable(CFAllocatorRef allocator, CFIndex capacity, const CFDictionaryKeyCallBacks *keyCallBacks, const CFDictionaryValueCallBacks *valueCallBacks)
{
	return (CFMutableDictionaryRef)dictAlloc(allocator, capacity, keyCallBacks, valueCallBacks);
}

CFMutableDictionaryRef CFDictionaryCreateMutableCopy(CFAllocatorRef allocator, CFIndex capacity, CFDictionaryRef theDict)
{
	const struct __CFDictionary *src = (const struct __CFDictionary *)theDict;
	struct __CFDictionary *d = dictAlloc(allocator, capacity > src->count ? capacity : src->count, src->hasCB ? &src->keyCB : NULL, src->hasCB ? &src->valueCB : NULL);
	for (CFIndex i = 0; i < src->count; i++) {
		d->keys[i] = (d->hasCB && d->keyCB.retain) ? d->keyCB.retain(allocator, src->keys[i]) : src->keys[i];
		d->values[i] = (d->hasCB && d->valueCB.retain) ? d->valueCB.retain(allocator, src->values[i]) : src->values[i];
	}
	d->count = src->count;
	return (CFMutableDictionaryRef)d;
}

CFIndex CFDictionaryGetCount(CFDictionaryRef theDict)
{
	return ((const struct __CFDictionary *)theDict)->count;
}

static CFIndex dictFind(const struct __CFDictionary *d, const void *key)
{
	for (CFIndex i = 0; i < d->count; i++)
		if (dictKeyEqual(d, d->keys[i], key))
			return i;
	return -1;
}

Boolean CFDictionaryContainsKey(CFDictionaryRef theDict, const void *key)
{
	return dictFind((const struct __CFDictionary *)theDict, key) != -1;
}

Boolean CFDictionaryContainsValue(CFDictionaryRef theDict, const void *value)
{
	const struct __CFDictionary *d = (const struct __CFDictionary *)theDict;
	for (CFIndex i = 0; i < d->count; i++) {
		Boolean eq = (d->hasCB && d->valueCB.equal) ? d->valueCB.equal(d->values[i], value) : (d->values[i] == value);
		if (eq)
			return true;
	}
	return false;
}

const void *CFDictionaryGetValue(CFDictionaryRef theDict, const void *key)
{
	const struct __CFDictionary *d = (const struct __CFDictionary *)theDict;
	CFIndex i = dictFind(d, key);
	return i == -1 ? NULL : d->values[i];
}

Boolean CFDictionaryGetValueIfPresent(CFDictionaryRef theDict, const void *key, const void **value)
{
	const struct __CFDictionary *d = (const struct __CFDictionary *)theDict;
	CFIndex i = dictFind(d, key);
	if (i == -1)
		return false;
	if (value)
		*value = d->values[i];
	return true;
}

void CFDictionaryGetKeysAndValues(CFDictionaryRef theDict, const void **keys, const void **values)
{
	const struct __CFDictionary *d = (const struct __CFDictionary *)theDict;
	if (keys)
		memcpy(keys, d->keys, sizeof(const void *) * (size_t)d->count);
	if (values)
		memcpy(values, d->values, sizeof(const void *) * (size_t)d->count);
}

void CFDictionaryApplyFunction(CFDictionaryRef theDict, CFDictionaryApplierFunction applier, void *context)
{
	const struct __CFDictionary *d = (const struct __CFDictionary *)theDict;
	for (CFIndex i = 0; i < d->count; i++)
		applier(d->keys[i], d->values[i], context);
}

static void dictEnsureCapacity(struct __CFDictionary *d, CFIndex needed)
{
	if (needed <= d->capacity)
		return;
	while (d->capacity < needed)
		d->capacity *= 2;
	d->keys = realloc(d->keys, sizeof(const void *) * (size_t)d->capacity);
	d->values = realloc(d->values, sizeof(const void *) * (size_t)d->capacity);
}

void CFDictionarySetValue(CFMutableDictionaryRef theDict, const void *key, const void *value)
{
	struct __CFDictionary *d = (struct __CFDictionary *)theDict;
	CFIndex i = dictFind(d, key);
	if (i != -1) {
		if (d->hasCB && d->valueCB.release)
			d->valueCB.release(kCFAllocatorDefault, d->values[i]);
		d->values[i] = (d->hasCB && d->valueCB.retain) ? d->valueCB.retain(kCFAllocatorDefault, value) : value;
		return;
	}
	dictEnsureCapacity(d, d->count + 1);
	d->keys[d->count] = (d->hasCB && d->keyCB.retain) ? d->keyCB.retain(kCFAllocatorDefault, key) : key;
	d->values[d->count] = (d->hasCB && d->valueCB.retain) ? d->valueCB.retain(kCFAllocatorDefault, value) : value;
	d->count++;
}

void CFDictionaryAddValue(CFMutableDictionaryRef theDict, const void *key, const void *value)
{
	struct __CFDictionary *d = (struct __CFDictionary *)theDict;
	if (dictFind(d, key) != -1)
		return;
	CFDictionarySetValue(theDict, key, value);
}

void CFDictionaryRemoveValue(CFMutableDictionaryRef theDict, const void *key)
{
	struct __CFDictionary *d = (struct __CFDictionary *)theDict;
	CFIndex i = dictFind(d, key);
	if (i == -1)
		return;
	if (d->hasCB) {
		if (d->keyCB.release)
			d->keyCB.release(kCFAllocatorDefault, d->keys[i]);
		if (d->valueCB.release)
			d->valueCB.release(kCFAllocatorDefault, d->values[i]);
	}
	memmove(d->keys + i, d->keys + i + 1, sizeof(const void *) * (size_t)(d->count - i - 1));
	memmove(d->values + i, d->values + i + 1, sizeof(const void *) * (size_t)(d->count - i - 1));
	d->count--;
}

void CFDictionaryRemoveAllValues(CFMutableDictionaryRef theDict)
{
	struct __CFDictionary *d = (struct __CFDictionary *)theDict;
	if (d->hasCB) {
		for (CFIndex i = 0; i < d->count; i++) {
			if (d->keyCB.release)
				d->keyCB.release(kCFAllocatorDefault, d->keys[i]);
			if (d->valueCB.release)
				d->valueCB.release(kCFAllocatorDefault, d->values[i]);
		}
	}
	d->count = 0;
}
