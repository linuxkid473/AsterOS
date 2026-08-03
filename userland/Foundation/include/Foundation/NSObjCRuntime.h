/* Copyright (c) 2026 Vihaan Nathan
 *
 * Fundamental Foundation typedefs every other header in this tree
 * depends on. Signatures match Apple's public NSObjCRuntime.h closely
 * enough for real .m client code to #import unmodified.
 */
#ifndef FOUNDATION_NSOBJCRUNTIME_H
#define FOUNDATION_NSOBJCRUNTIME_H

#include <objc/objc.h>
#include <objc/runtime.h>
#include <objc/message.h>
#include <stdint.h>
#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FOUNDATION_EXPORT extern
#define FOUNDATION_INLINE static __inline__

typedef long NSInteger;
typedef unsigned long NSUInteger;

#define NSIntegerMax LONG_MAX
#define NSIntegerMin LONG_MIN
#define NSUIntegerMax ULONG_MAX

typedef double NSTimeInterval;

typedef struct _NSRange {
	NSUInteger location;
	NSUInteger length;
} NSRange;

FOUNDATION_INLINE NSRange NSMakeRange(NSUInteger loc, NSUInteger len)
{
	NSRange r;
	r.location = loc;
	r.length = len;
	return r;
}

typedef enum {
	NSOrderedAscending = -1,
	NSOrderedSame = 0,
	NSOrderedDescending = 1
} NSComparisonResult;

typedef NSUInteger NSStringEncoding;
enum {
	NSASCIIStringEncoding = 1,
	NSUTF8StringEncoding = 4,
};

/* Zones are vestigial even on real modern Darwin (retired along with the
 * GC-era zone allocator this OS never had to begin with) -- an opaque,
 * never-dereferenced type is correct here, not a stub: every
 * -copyWithZone:/-mutableCopyWithZone: in this tree ignores its zone
 * argument, matching real current Foundation's own behavior. */
typedef struct _NSZone NSZone;

FOUNDATION_EXPORT const char *NSStringFromClassName(Class cls);

#ifdef __cplusplus
}
#endif

#endif /* FOUNDATION_NSOBJCRUNTIME_H */
