/* Copyright (c) 2026 Vihaan Nathan
 *
 * See CFSet.h: same linear-array O(n) lookup tradeoff as CFDictionary.c.
 */
#include "CFInternal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

struct __CFSet {
	CFRuntimeBase base;
	const void **values;
	CFIndex count;
	CFIndex capacity;
	CFSetCallBacks callbacks;
	Boolean hasCB;
};

static CFTypeID g_setTypeID;
static pthread_once_t g_setOnce = PTHREAD_ONCE_INIT;

static const void *skRetain(CFAllocatorRef a, const void *v) { (void)a; return CFRetain(v); }
static void skRelease(CFAllocatorRef a, const void *v) { (void)a; CFRelease(v); }
static CFStringRef skCopyDesc(const void *v) { return CFCopyDescription(v); }
static Boolean skEqual(const void *v1, const void *v2) { return CFEqual(v1, v2); }
static CFHashCode skHash(const void *v) { return CFHash(v); }

const CFSetCallBacks kCFTypeSetCallBacks = {
	.version = 0, .retain = skRetain, .release = skRelease,
	.copyDescription = skCopyDesc, .equal = skEqual, .hash = skHash,
};

static Boolean setValEqual(const struct __CFSet *s, const void *a, const void *b)
{
	return (s->hasCB && s->callbacks.equal) ? s->callbacks.equal(a, b) : (a == b);
}

static void setFinalize(CFTypeRef cf)
{
	struct __CFSet *s = (struct __CFSet *)cf;
	if (s->hasCB && s->callbacks.release)
		for (CFIndex i = 0; i < s->count; i++)
			s->callbacks.release(kCFAllocatorDefault, s->values[i]);
	free(s->values);
}

static Boolean setEqual(CFTypeRef cf1, CFTypeRef cf2)
{
	const struct __CFSet *a = cf1, *b = cf2;
	if (a->count != b->count)
		return false;
	for (CFIndex i = 0; i < a->count; i++)
		if (!CFSetContainsValue((CFSetRef)b, a->values[i]))
			return false;
	return true;
}

static CFHashCode setHash(CFTypeRef cf)
{
	return (CFHashCode)((const struct __CFSet *)cf)->count;
}

static CFStringRef setCopyDesc(CFTypeRef cf)
{
	const struct __CFSet *s = cf;
	CFMutableStringRef desc = CFStringCreateMutable(kCFAllocatorDefault, 0);
	CFStringAppendCString(desc, "{(", kCFStringEncodingUTF8);
	for (CFIndex i = 0; i < s->count; i++) {
		if (i > 0)
			CFStringAppendCString(desc, ", ", kCFStringEncodingUTF8);
		CFStringRef vd = (s->hasCB && s->callbacks.copyDescription) ? s->callbacks.copyDescription(s->values[i]) : NULL;
		if (vd) {
			CFStringAppend(desc, vd);
			CFRelease(vd);
		} else {
			char buf[32];
			snprintf(buf, sizeof(buf), "%p", s->values[i]);
			CFStringAppendCString(desc, buf, kCFStringEncodingUTF8);
		}
	}
	CFStringAppendCString(desc, ")}", kCFStringEncodingUTF8);
	return (CFStringRef)desc;
}

static void setInit(void)
{
	static const CFRuntimeClass cls = {
		.className = "CFSet",
		.finalize = setFinalize,
		.equal = setEqual,
		.hash = setHash,
		.copyFormattingDesc = setCopyDesc,
	};
	g_setTypeID = _CFRuntimeRegisterClass(&cls);
}

CFTypeID CFSetGetTypeID(void)
{
	pthread_once(&g_setOnce, setInit);
	return g_setTypeID;
}

static struct __CFSet *setAlloc(CFAllocatorRef allocator, CFIndex capacity, const CFSetCallBacks *callBacks)
{
	CFSetGetTypeID();
	struct __CFSet *s = (struct __CFSet *)_CFRuntimeCreateInstance(allocator, g_setTypeID, sizeof(struct __CFSet) - sizeof(CFRuntimeBase));
	if (capacity < 4)
		capacity = 4;
	s->capacity = capacity;
	s->values = malloc(sizeof(const void *) * (size_t)capacity);
	if (callBacks) {
		s->callbacks = *callBacks;
		s->hasCB = true;
	}
	return s;
}

static CFIndex setFind(const struct __CFSet *s, const void *value)
{
	for (CFIndex i = 0; i < s->count; i++)
		if (setValEqual(s, s->values[i], value))
			return i;
	return -1;
}

CFSetRef CFSetCreate(CFAllocatorRef allocator, const void **values, CFIndex numValues, const CFSetCallBacks *callBacks)
{
	struct __CFSet *s = setAlloc(allocator, numValues, callBacks);
	for (CFIndex i = 0; i < numValues; i++) {
		if (setFind(s, values[i]) != -1)
			continue;
		s->values[s->count++] = (s->hasCB && s->callbacks.retain) ? s->callbacks.retain(allocator, values[i]) : values[i];
	}
	return (CFSetRef)s;
}

CFSetRef CFSetCreateCopy(CFAllocatorRef allocator, CFSetRef theSet)
{
	const struct __CFSet *src = (const struct __CFSet *)theSet;
	return CFSetCreate(allocator, src->values, src->count, src->hasCB ? &src->callbacks : NULL);
}

CFMutableSetRef CFSetCreateMutable(CFAllocatorRef allocator, CFIndex capacity, const CFSetCallBacks *callBacks)
{
	return (CFMutableSetRef)setAlloc(allocator, capacity, callBacks);
}

CFMutableSetRef CFSetCreateMutableCopy(CFAllocatorRef allocator, CFIndex capacity, CFSetRef theSet)
{
	const struct __CFSet *src = (const struct __CFSet *)theSet;
	struct __CFSet *s = setAlloc(allocator, capacity > src->count ? capacity : src->count, src->hasCB ? &src->callbacks : NULL);
	for (CFIndex i = 0; i < src->count; i++)
		s->values[i] = (s->hasCB && s->callbacks.retain) ? s->callbacks.retain(allocator, src->values[i]) : src->values[i];
	s->count = src->count;
	return (CFMutableSetRef)s;
}

CFIndex CFSetGetCount(CFSetRef theSet)
{
	return ((const struct __CFSet *)theSet)->count;
}

Boolean CFSetContainsValue(CFSetRef theSet, const void *value)
{
	return setFind((const struct __CFSet *)theSet, value) != -1;
}

const void *CFSetGetValue(CFSetRef theSet, const void *value)
{
	const struct __CFSet *s = (const struct __CFSet *)theSet;
	CFIndex i = setFind(s, value);
	return i == -1 ? NULL : s->values[i];
}

Boolean CFSetGetValueIfPresent(CFSetRef theSet, const void *candidate, const void **value)
{
	const struct __CFSet *s = (const struct __CFSet *)theSet;
	CFIndex i = setFind(s, candidate);
	if (i == -1)
		return false;
	if (value)
		*value = s->values[i];
	return true;
}

void CFSetGetValues(CFSetRef theSet, const void **values)
{
	const struct __CFSet *s = (const struct __CFSet *)theSet;
	memcpy(values, s->values, sizeof(const void *) * (size_t)s->count);
}

void CFSetApplyFunction(CFSetRef theSet, CFSetApplierFunction applier, void *context)
{
	const struct __CFSet *s = (const struct __CFSet *)theSet;
	for (CFIndex i = 0; i < s->count; i++)
		applier(s->values[i], context);
}

static void setEnsureCapacity(struct __CFSet *s, CFIndex needed)
{
	if (needed <= s->capacity)
		return;
	while (s->capacity < needed)
		s->capacity *= 2;
	s->values = realloc(s->values, sizeof(const void *) * (size_t)s->capacity);
}

void CFSetAddValue(CFMutableSetRef theSet, const void *value)
{
	struct __CFSet *s = (struct __CFSet *)theSet;
	if (setFind(s, value) != -1)
		return;
	setEnsureCapacity(s, s->count + 1);
	s->values[s->count++] = (s->hasCB && s->callbacks.retain) ? s->callbacks.retain(kCFAllocatorDefault, value) : value;
}

void CFSetSetValue(CFMutableSetRef theSet, const void *value)
{
	struct __CFSet *s = (struct __CFSet *)theSet;
	CFIndex i = setFind(s, value);
	if (i != -1) {
		if (s->hasCB && s->callbacks.release)
			s->callbacks.release(kCFAllocatorDefault, s->values[i]);
		s->values[i] = (s->hasCB && s->callbacks.retain) ? s->callbacks.retain(kCFAllocatorDefault, value) : value;
		return;
	}
	CFSetAddValue(theSet, value);
}

void CFSetRemoveValue(CFMutableSetRef theSet, const void *value)
{
	struct __CFSet *s = (struct __CFSet *)theSet;
	CFIndex i = setFind(s, value);
	if (i == -1)
		return;
	if (s->hasCB && s->callbacks.release)
		s->callbacks.release(kCFAllocatorDefault, s->values[i]);
	memmove(s->values + i, s->values + i + 1, sizeof(const void *) * (size_t)(s->count - i - 1));
	s->count--;
}

void CFSetRemoveAllValues(CFMutableSetRef theSet)
{
	struct __CFSet *s = (struct __CFSet *)theSet;
	if (s->hasCB && s->callbacks.release)
		for (CFIndex i = 0; i < s->count; i++)
			s->callbacks.release(kCFAllocatorDefault, s->values[i]);
	s->count = 0;
}
