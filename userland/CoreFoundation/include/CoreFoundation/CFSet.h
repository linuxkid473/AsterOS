/* Copyright (c) 2026 Vihaan Nathan
 *
 * v1 simplification: same linear-array storage tradeoff as CFDictionary.h.
 */
#ifndef __COREFOUNDATION_CFSET_H__
#define __COREFOUNDATION_CFSET_H__

#include <CoreFoundation/CFBase.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef const struct __CFSet *CFSetRef;
typedef struct __CFSet *CFMutableSetRef;

typedef const void *(*CFSetRetainCallBack)(CFAllocatorRef allocator, const void *value);
typedef void (*CFSetReleaseCallBack)(CFAllocatorRef allocator, const void *value);
typedef CFStringRef (*CFSetCopyDescriptionCallBack)(const void *value);
typedef Boolean (*CFSetEqualCallBack)(const void *value1, const void *value2);
typedef CFHashCode (*CFSetHashCallBack)(const void *value);

typedef struct {
	CFIndex version;
	CFSetRetainCallBack retain;
	CFSetReleaseCallBack release;
	CFSetCopyDescriptionCallBack copyDescription;
	CFSetEqualCallBack equal;
	CFSetHashCallBack hash;
} CFSetCallBacks;

CF_EXPORT const CFSetCallBacks kCFTypeSetCallBacks;

typedef void (*CFSetApplierFunction)(const void *value, void *context);

CF_EXPORT CFTypeID CFSetGetTypeID(void);

CF_EXPORT CFSetRef CFSetCreate(CFAllocatorRef allocator, const void **values, CFIndex numValues, const CFSetCallBacks *callBacks);
CF_EXPORT CFSetRef CFSetCreateCopy(CFAllocatorRef allocator, CFSetRef theSet);
CF_EXPORT CFMutableSetRef CFSetCreateMutable(CFAllocatorRef allocator, CFIndex capacity, const CFSetCallBacks *callBacks);
CF_EXPORT CFMutableSetRef CFSetCreateMutableCopy(CFAllocatorRef allocator, CFIndex capacity, CFSetRef theSet);

CF_EXPORT CFIndex CFSetGetCount(CFSetRef theSet);
CF_EXPORT Boolean CFSetContainsValue(CFSetRef theSet, const void *value);
CF_EXPORT const void *CFSetGetValue(CFSetRef theSet, const void *value);
CF_EXPORT Boolean CFSetGetValueIfPresent(CFSetRef theSet, const void *candidate, const void **value);
CF_EXPORT void CFSetGetValues(CFSetRef theSet, const void **values);
CF_EXPORT void CFSetApplyFunction(CFSetRef theSet, CFSetApplierFunction applier, void *context);

CF_EXPORT void CFSetAddValue(CFMutableSetRef theSet, const void *value);
CF_EXPORT void CFSetSetValue(CFMutableSetRef theSet, const void *value);
CF_EXPORT void CFSetRemoveValue(CFMutableSetRef theSet, const void *value);
CF_EXPORT void CFSetRemoveAllValues(CFMutableSetRef theSet);

#ifdef __cplusplus
}
#endif

#endif /* __COREFOUNDATION_CFSET_H__ */
