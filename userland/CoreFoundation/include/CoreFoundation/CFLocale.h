/* Copyright (c) 2026 Vihaan Nathan
 *
 * "en_US_POSIX"-only: this tree's libc has exactly one locale (see
 * userland/libc/src/locale.c's own header comment -- setlocale only
 * ever accepts "C"/"POSIX"/""). CFLocaleCopyCurrent/CopySystem both
 * return the fixed en_US_POSIX identifier (the real, standard Cocoa
 * placeholder for "no real locale data, POSIX-neutral behavior"), and
 * CFLocaleCreate accepts any identifier string purely as a label with
 * no behavioral effect -- no number/date formatting, no collation.
 * Documented, not a silent gap.
 */
#ifndef __COREFOUNDATION_CFLOCALE_H__
#define __COREFOUNDATION_CFLOCALE_H__

#include <CoreFoundation/CFBase.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef const struct __CFLocale *CFLocaleRef;

CF_EXPORT CFTypeID CFLocaleGetTypeID(void);
CF_EXPORT CFLocaleRef CFLocaleCreate(CFAllocatorRef allocator, CFStringRef localeIdentifier);
CF_EXPORT CFLocaleRef CFLocaleCopyCurrent(void);
CF_EXPORT CFLocaleRef CFLocaleGetSystem(void);
CF_EXPORT CFStringRef CFLocaleGetIdentifier(CFLocaleRef locale);

#ifdef __cplusplus
}
#endif

#endif /* __COREFOUNDATION_CFLOCALE_H__ */
