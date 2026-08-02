/* Copyright (c) 2026 Vihaan Nathan
 *
 * Core CF type system: CFTypeRef/CFTypeID, CFAllocator, and the
 * retain/release/equal/hash/description primitives every other CF type
 * builds on. Signatures match real CoreFoundation's public CFBase.h so
 * unmodified client code compiles against this; the object model behind
 * them (CFInternal.h's CFRuntimeBase/CFRuntimeClass) is our own, sized
 * for what this OS actually needs rather than ported from Apple's.
 */
#ifndef __COREFOUNDATION_CFBASE_H__
#define __COREFOUNDATION_CFBASE_H__

#include <MacTypes.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CF_EXPORT extern
#define CF_INLINE static __inline__

typedef unsigned long CFTypeID;
typedef unsigned long CFOptionFlags;
typedef unsigned long CFHashCode;
typedef signed long CFIndex;

typedef const void *CFTypeRef;

/* Forward-declared here, defined in full by CFString.h -- every CF type
 * that can describe itself (CFCopyDescription) needs CFStringRef before
 * CFString.h's own contents are relevant. */
typedef const struct __CFString *CFStringRef;
typedef struct __CFString *CFMutableStringRef;

typedef struct {
	CFIndex location;
	CFIndex length;
} CFRange;

CF_INLINE CFRange CFRangeMake(CFIndex loc, CFIndex len)
{
	CFRange r;
	r.location = loc;
	r.length = len;
	return r;
}

typedef enum {
	kCFCompareLessThan = -1,
	kCFCompareEqualTo = 0,
	kCFCompareGreaterThan = 1
} CFComparisonResult;

typedef CFComparisonResult (*CFComparatorFunction)(const void *val1, const void *val2, void *context);

/* ---- CFAllocator ----
 *
 * v1 simplification: there is no support for custom allocator contexts
 * (CFAllocatorCreate/CFAllocatorContext callbacks). Every named allocator
 * below aliases the same singleton, which is backed directly by the C
 * library's malloc/realloc/free -- documented, not an oversight, same
 * spirit as pthread's spin-based mutex being a deliberate v1 cut. Client
 * code that only ever passes kCFAllocatorDefault (the overwhelming
 * majority of real CF callers) works unmodified.
 */
typedef const struct __CFAllocator *CFAllocatorRef;

CF_EXPORT const CFAllocatorRef kCFAllocatorDefault;
CF_EXPORT const CFAllocatorRef kCFAllocatorSystemDefault;
CF_EXPORT const CFAllocatorRef kCFAllocatorMalloc;
CF_EXPORT const CFAllocatorRef kCFAllocatorNull;

CF_EXPORT CFTypeID CFAllocatorGetTypeID(void);
CF_EXPORT void *CFAllocatorAllocate(CFAllocatorRef allocator, CFIndex size, CFOptionFlags hint);
CF_EXPORT void *CFAllocatorReallocate(CFAllocatorRef allocator, void *ptr, CFIndex newsize, CFOptionFlags hint);
CF_EXPORT void CFAllocatorDeallocate(CFAllocatorRef allocator, void *ptr);
CF_EXPORT CFAllocatorRef CFAllocatorGetDefault(void);
CF_EXPORT void CFAllocatorSetDefault(CFAllocatorRef allocator);

/* ---- CFNull ---- */
typedef const struct __CFNull *CFNullRef;
CF_EXPORT const CFNullRef kCFNull;
CF_EXPORT CFTypeID CFNullGetTypeID(void);

/* ---- object model ---- */
CF_EXPORT CFTypeID CFGetTypeID(CFTypeRef cf);
CF_EXPORT CFStringRef CFCopyTypeIDDescription(CFTypeID type_id);
CF_EXPORT CFTypeRef CFRetain(CFTypeRef cf);
CF_EXPORT void CFRelease(CFTypeRef cf);
CF_EXPORT CFIndex CFGetRetainCount(CFTypeRef cf);
CF_EXPORT Boolean CFEqual(CFTypeRef cf1, CFTypeRef cf2);
CF_EXPORT CFHashCode CFHash(CFTypeRef cf);
CF_EXPORT CFStringRef CFCopyDescription(CFTypeRef cf);
CF_EXPORT CFAllocatorRef CFGetAllocator(CFTypeRef cf);
CF_EXPORT void CFShow(CFTypeRef obj);

#ifdef __cplusplus
}
#endif

#endif /* __COREFOUNDATION_CFBASE_H__ */
