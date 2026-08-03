/* Copyright (c) 2026 Vihaan Nathan
 *
 * Filesystem URLs only (per the task this phase was scoped from) --
 * absolute POSIX paths, "file://" scheme. No relative-URL resolution
 * against a base URL, no percent-encoding/decoding, no http(s)/other
 * schemes, no query/fragment parsing. Documented v1 cut, not silent --
 * see docs/architecture.md.
 */
#ifndef __COREFOUNDATION_CFURL_H__
#define __COREFOUNDATION_CFURL_H__

#include <CoreFoundation/CFBase.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef const struct __CFURL *CFURLRef;

typedef CFIndex CFURLPathStyle;
enum {
	kCFURLPOSIXPathStyle = 0,
};

CF_EXPORT CFTypeID CFURLGetTypeID(void);
CF_EXPORT CFURLRef CFURLCreateWithFileSystemPath(CFAllocatorRef allocator, CFStringRef filePath, CFURLPathStyle pathStyle, Boolean isDirectory);
CF_EXPORT CFURLRef CFURLCreateWithString(CFAllocatorRef allocator, CFStringRef URLString, CFURLRef baseURL);
CF_EXPORT CFStringRef CFURLGetString(CFURLRef url);
CF_EXPORT CFStringRef CFURLCopyFileSystemPath(CFURLRef url, CFURLPathStyle pathStyle);
CF_EXPORT CFStringRef CFURLCopyLastPathComponent(CFURLRef url);
CF_EXPORT CFStringRef CFURLCopyPathExtension(CFURLRef url);
CF_EXPORT Boolean CFURLHasDirectoryPath(CFURLRef url);

#ifdef __cplusplus
}
#endif

#endif /* __COREFOUNDATION_CFURL_H__ */
