/* Copyright (c) 2026 Vihaan Nathan -- see NSCFBridge.h */
#include "NSCFBridge.h"
#include <objc/runtime.h>
#include <objc/message.h>
#include <stdio.h>
#include <stdlib.h>

id NSCFBridge_retain(id self)
{
	return (id)CFRetain((CFTypeRef)self);
}

void NSCFBridge_release(id self)
{
	CFRelease((CFTypeRef)self);
}

NSUInteger NSCFBridge_retainCount(id self)
{
	return (NSUInteger)CFGetRetainCount((CFTypeRef)self);
}

BOOL NSCFBridge_isEqual(id self, id other)
{
	if (!other) {
		return NO;
	}
	return CFEqual((CFTypeRef)self, (CFTypeRef)other) ? YES : NO;
}

NSUInteger NSCFBridge_hash(id self)
{
	return (NSUInteger)CFHash((CFTypeRef)self);
}

NSString *NSCFBridge_description(id self)
{
	/* CFCopyDescription hands back a +1 reference; -description is
	 * expected to return an autoreleased (not caller-owned) object. */
	id result = (id)CFCopyDescription((CFTypeRef)self);
	((id (*)(id, SEL))objc_msgSend)(result, sel_registerName("autorelease"));
	return (NSString *)result;
}

void NSCFBridge_deallocGuard(id self)
{
	(void)self;
	fprintf(stderr, "libFoundation: -dealloc sent directly to a toll-free-bridged object -- always release it instead, CFRelease frees it at refcount 0\n");
	abort();
}

/* ---- collection element callbacks -- see NSCFBridge.h ---- */

static const void *
objcRetainCallback(CFAllocatorRef allocator, const void *value)
{
	(void)allocator;
	return (const void *)((id (*)(id, SEL))objc_msgSend)((id)value, sel_registerName("retain"));
}

static const void *
objcCopyCallback(CFAllocatorRef allocator, const void *value)
{
	(void)allocator;
	return (const void *)((id (*)(id, SEL))objc_msgSend)((id)value, sel_registerName("copy"));
}

static void
objcReleaseCallback(CFAllocatorRef allocator, const void *value)
{
	(void)allocator;
	((void (*)(id, SEL))objc_msgSend)((id)value, sel_registerName("release"));
}

static Boolean
objcEqualCallback(const void *value1, const void *value2)
{
	return ((BOOL (*)(id, SEL, id))objc_msgSend)((id)value1, sel_registerName("isEqual:"), (id)value2) ? true : false;
}

static CFHashCode
objcHashCallback(const void *value)
{
	return (CFHashCode)((NSUInteger (*)(id, SEL))objc_msgSend)((id)value, sel_registerName("hash"));
}

static CFStringRef
objcCopyDescriptionCallback(const void *value)
{
	/* CF's copyDescription callback contract expects a +1 reference
	 * back (CF releases it itself); -description returns autoreleased,
	 * so retain once more before handing it back. */
	id desc = ((id (*)(id, SEL))objc_msgSend)((id)value, sel_registerName("description"));
	return (CFStringRef)((id (*)(id, SEL))objc_msgSend)(desc, sel_registerName("retain"));
}

const CFArrayCallBacks kNSObjectArrayCallBacks = {
	.version = 0,
	.retain = objcRetainCallback,
	.release = objcReleaseCallback,
	.copyDescription = objcCopyDescriptionCallback,
	.equal = objcEqualCallback,
};

const CFDictionaryKeyCallBacks kNSObjectDictionaryKeyCallBacks = {
	.version = 0,
	.retain = objcCopyCallback,	/* keys are copied, not retained -- real NSDictionary's NSCopying contract */
	.release = objcReleaseCallback,
	.copyDescription = objcCopyDescriptionCallback,
	.equal = objcEqualCallback,
	.hash = objcHashCallback,
};

const CFDictionaryValueCallBacks kNSObjectDictionaryValueCallBacks = {
	.version = 0,
	.retain = objcRetainCallback,
	.release = objcReleaseCallback,
	.copyDescription = objcCopyDescriptionCallback,
	.equal = objcEqualCallback,
};

const CFSetCallBacks kNSObjectSetCallBacks = {
	.version = 0,
	.retain = objcRetainCallback,
	.release = objcReleaseCallback,
	.copyDescription = objcCopyDescriptionCallback,
	.equal = objcEqualCallback,
	.hash = objcHashCallback,
};

void
NSCFBridge_formatDouble(double d, char *out, size_t outsize)
{
	int neg = d < 0.0;
	if (neg) {
		d = -d;
	}
	unsigned long long intPart = (unsigned long long)d;
	double frac = d - (double)intPart;
	char fracDigits[18];
	int nfrac = 0;
	for (int i = 0; i < 15 && frac > 0.0 && nfrac < (int)sizeof(fracDigits) - 1; i++) {
		frac *= 10.0;
		int digit = (int)frac;
		fracDigits[nfrac++] = (char)('0' + digit);
		frac -= digit;
	}
	while (nfrac > 0 && fracDigits[nfrac - 1] == '0') {
		nfrac--;
	}
	fracDigits[nfrac] = '\0';
	if (nfrac == 0) {
		snprintf(out, outsize, "%s%llu.0", neg ? "-" : "", intPart);
	} else {
		snprintf(out, outsize, "%s%llu.%s", neg ? "-" : "", intPart, fracDigits);
	}
}
