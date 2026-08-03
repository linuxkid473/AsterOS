/* Copyright (c) 2026 Vihaan Nathan
 *
 * UTC-only: this tree's libc has no real IANA tzdata / TZ-env parsing at
 * all (userland/libc/src/time.c's localtime_r is a plain alias for
 * gmtime_r, tm_gmtoff always 0, tm_zone always "UTC") -- CFTimeZone
 * honestly reflects that rather than pretending to support named zones
 * it can't actually compute an offset for. CFTimeZoneCreateWithName
 * only ever succeeds for "UTC"/"GMT"; every other name returns NULL.
 * Documented, not a silent gap -- see docs/architecture.md.
 */
#ifndef __COREFOUNDATION_CFTIMEZONE_H__
#define __COREFOUNDATION_CFTIMEZONE_H__

#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFDate.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef const struct __CFTimeZone *CFTimeZoneRef;

CF_EXPORT CFTypeID CFTimeZoneGetTypeID(void);
CF_EXPORT CFTimeZoneRef CFTimeZoneCreateWithName(CFAllocatorRef allocator, CFStringRef name, Boolean tryAbbrev);
CF_EXPORT CFTimeZoneRef CFTimeZoneCopySystem(void);
CF_EXPORT CFTimeZoneRef CFTimeZoneCopyDefault(void);
CF_EXPORT CFStringRef CFTimeZoneGetName(CFTimeZoneRef tz);
CF_EXPORT CFTimeInterval CFTimeZoneGetSecondsFromGMT(CFTimeZoneRef tz, CFAbsoluteTime at);

#ifdef __cplusplus
}
#endif

#endif /* __COREFOUNDATION_CFTIMEZONE_H__ */
