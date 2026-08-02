/* Copyright (c) 2026 Vihaan Nathan
 *
 * v1 simplification: backed by a linear key/value array (O(n) lookup),
 * not a real hash table -- same tradeoff already made for pthread TSD
 * lookup in this codebase (see TODO.md). Callback-driven retain/release/
 * equal semantics are real; only the storage strategy is simplified.
 */
#ifndef __COREFOUNDATION_CFDICTIONARY_H__
#define __COREFOUNDATION_CFDICTIONARY_H__

#include <CoreFoundation/CFBase.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef const struct __CFDictionary *CFDictionaryRef;
typedef struct __CFDictionary *CFMutableDictionaryRef;

typedef const void *(*CFDictionaryRetainCallBack)(CFAllocatorRef allocator, const void *value);
typedef void (*CFDictionaryReleaseCallBack)(CFAllocatorRef allocator, const void *value);
typedef CFStringRef (*CFDictionaryCopyDescriptionCallBack)(const void *value);
typedef Boolean (*CFDictionaryEqualCallBack)(const void *value1, const void *value2);
typedef CFHashCode (*CFDictionaryHashCallBack)(const void *value);

typedef struct {
	CFIndex version;
	CFDictionaryRetainCallBack retain;
	CFDictionaryReleaseCallBack release;
	CFDictionaryCopyDescriptionCallBack copyDescription;
	CFDictionaryEqualCallBack equal;
	CFDictionaryHashCallBack hash;
} CFDictionaryKeyCallBacks;

typedef struct {
	CFIndex version;
	CFDictionaryRetainCallBack retain;
	CFDictionaryReleaseCallBack release;
	CFDictionaryCopyDescriptionCallBack copyDescription;
	CFDictionaryEqualCallBack equal;
} CFDictionaryValueCallBacks;

CF_EXPORT const CFDictionaryKeyCallBacks kCFTypeDictionaryKeyCallBacks;
CF_EXPORT const CFDictionaryValueCallBacks kCFTypeDictionaryValueCallBacks;

typedef void (*CFDictionaryApplierFunction)(const void *key, const void *value, void *context);

CF_EXPORT CFTypeID CFDictionaryGetTypeID(void);

CF_EXPORT CFDictionaryRef CFDictionaryCreate(CFAllocatorRef allocator, const void **keys, const void **values, CFIndex numValues, const CFDictionaryKeyCallBacks *keyCallBacks, const CFDictionaryValueCallBacks *valueCallBacks);
CF_EXPORT CFDictionaryRef CFDictionaryCreateCopy(CFAllocatorRef allocator, CFDictionaryRef theDict);
CF_EXPORT CFMutableDictionaryRef CFDictionaryCreateMutable(CFAllocatorRef allocator, CFIndex capacity, const CFDictionaryKeyCallBacks *keyCallBacks, const CFDictionaryValueCallBacks *valueCallBacks);
CF_EXPORT CFMutableDictionaryRef CFDictionaryCreateMutableCopy(CFAllocatorRef allocator, CFIndex capacity, CFDictionaryRef theDict);

CF_EXPORT CFIndex CFDictionaryGetCount(CFDictionaryRef theDict);
CF_EXPORT Boolean CFDictionaryContainsKey(CFDictionaryRef theDict, const void *key);
CF_EXPORT Boolean CFDictionaryContainsValue(CFDictionaryRef theDict, const void *value);
CF_EXPORT const void *CFDictionaryGetValue(CFDictionaryRef theDict, const void *key);
CF_EXPORT Boolean CFDictionaryGetValueIfPresent(CFDictionaryRef theDict, const void *key, const void **value);
CF_EXPORT void CFDictionaryGetKeysAndValues(CFDictionaryRef theDict, const void **keys, const void **values);
CF_EXPORT void CFDictionaryApplyFunction(CFDictionaryRef theDict, CFDictionaryApplierFunction applier, void *context);

CF_EXPORT void CFDictionarySetValue(CFMutableDictionaryRef theDict, const void *key, const void *value);
CF_EXPORT void CFDictionaryAddValue(CFMutableDictionaryRef theDict, const void *key, const void *value);
CF_EXPORT void CFDictionaryRemoveValue(CFMutableDictionaryRef theDict, const void *key);
CF_EXPORT void CFDictionaryRemoveAllValues(CFMutableDictionaryRef theDict);

#ifdef __cplusplus
}
#endif

#endif /* __COREFOUNDATION_CFDICTIONARY_H__ */
