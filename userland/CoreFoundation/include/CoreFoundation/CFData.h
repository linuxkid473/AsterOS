/* Copyright (c) 2026 Vihaan Nathan */
#ifndef __COREFOUNDATION_CFDATA_H__
#define __COREFOUNDATION_CFDATA_H__

#include <CoreFoundation/CFBase.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef const struct __CFData *CFDataRef;
typedef struct __CFData *CFMutableDataRef;

CF_EXPORT CFTypeID CFDataGetTypeID(void);

CF_EXPORT CFDataRef CFDataCreate(CFAllocatorRef allocator, const UInt8 *bytes, CFIndex length);
CF_EXPORT CFDataRef CFDataCreateCopy(CFAllocatorRef allocator, CFDataRef theData);
CF_EXPORT CFMutableDataRef CFDataCreateMutable(CFAllocatorRef allocator, CFIndex capacity);
CF_EXPORT CFMutableDataRef CFDataCreateMutableCopy(CFAllocatorRef allocator, CFIndex capacity, CFDataRef theData);

CF_EXPORT CFIndex CFDataGetLength(CFDataRef theData);
CF_EXPORT const UInt8 *CFDataGetBytePtr(CFDataRef theData);
CF_EXPORT UInt8 *CFDataGetMutableBytePtr(CFMutableDataRef theData);
CF_EXPORT void CFDataGetBytes(CFDataRef theData, CFRange range, UInt8 *buffer);

CF_EXPORT void CFDataAppendBytes(CFMutableDataRef theData, const UInt8 *bytes, CFIndex length);
CF_EXPORT void CFDataDeleteBytes(CFMutableDataRef theData, CFRange range);
CF_EXPORT void CFDataReplaceBytes(CFMutableDataRef theData, CFRange range, const UInt8 *newBytes, CFIndex newLength);
CF_EXPORT void CFDataSetLength(CFMutableDataRef theData, CFIndex length);
CF_EXPORT void CFDataIncreaseLength(CFMutableDataRef theData, CFIndex extraLength);

#ifdef __cplusplus
}
#endif

#endif /* __COREFOUNDATION_CFDATA_H__ */
