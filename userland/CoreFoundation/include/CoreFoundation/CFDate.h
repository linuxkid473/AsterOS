/* Copyright (c) 2026 Vihaan Nathan
 *
 * Added for Foundation's NSDate (see TODO.md's Foundation phase) --
 * CFDate itself is genuinely small: a CFRuntimeBase plus one double.
 */
#ifndef __COREFOUNDATION_CFDATE_H__
#define __COREFOUNDATION_CFDATE_H__

#include <CoreFoundation/CFBase.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef double CFAbsoluteTime;
typedef CFAbsoluteTime CFTimeInterval;

/* Seconds between 1970-01-01 00:00:00 UTC (this tree's libc epoch, see
 * userland/libc/src/time.c) and 2001-01-01 00:00:00 UTC (CF's reference
 * date) -- the standard real constant, not something specific to this
 * implementation. */
#define kCFAbsoluteTimeIntervalSince1970 978307200.0

CF_EXPORT CFAbsoluteTime CFAbsoluteTimeGetCurrent(void);

typedef const struct __CFDate *CFDateRef;

CF_EXPORT CFTypeID CFDateGetTypeID(void);
CF_EXPORT CFDateRef CFDateCreate(CFAllocatorRef allocator, CFAbsoluteTime at);
CF_EXPORT CFAbsoluteTime CFDateGetAbsoluteTime(CFDateRef theDate);
CF_EXPORT CFTimeInterval CFDateGetTimeIntervalSinceDate(CFDateRef theDate, CFDateRef otherDate);
CF_EXPORT CFComparisonResult CFDateCompare(CFDateRef theDate, CFDateRef otherDate, void *context);

#ifdef __cplusplus
}
#endif

#endif /* __COREFOUNDATION_CFDATE_H__ */
