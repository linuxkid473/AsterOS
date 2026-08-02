/* Copyright (c) 2026 Vihaan Nathan
 *
 * Private object model every CF type in this tree is built on. Not part
 * of the public API (real CF's CFRuntime.h is private too, for what
 * it's worth) -- this is our own layout, sized for what this OS needs:
 * a fixed, compile-time-known set of built-in classes, each identified
 * by a CFTypeID handed out at first-use via pthread_once (mirroring
 * real CF's own dispatch_once-per-type pattern for GetTypeID functions).
 *
 * Every concrete type's struct embeds a CFRuntimeBase as its first
 * member, so a CFTypeRef is always safely castable back to
 * CFRuntimeBase* regardless of which concrete type it actually is --
 * the same "base struct is the first field" idiom xnu itself uses
 * throughout (vm_map_entry, ipc_port, etc).
 */
#ifndef COREFOUNDATION_CFINTERNAL_H
#define COREFOUNDATION_CFINTERNAL_H

#include <CoreFoundation/CoreFoundation.h>

#define CF_MAX_RUNTIME_CLASSES 32

typedef struct __CFRuntimeBase {
	CFTypeID typeID;
	Boolean isConstant;	/* statically-allocated singletons (kCFBooleanTrue, kCFNull, ...): CFRetain/CFRelease are no-ops */
	volatile CFIndex retainCount;	/* touched only via __atomic_* builtins -- see pthread.c for the same convention */
} CFRuntimeBase;

typedef struct {
	const char *className;
	void (*finalize)(CFTypeRef cf);
	Boolean (*equal)(CFTypeRef cf1, CFTypeRef cf2);
	CFHashCode (*hash)(CFTypeRef cf);
	CFStringRef (*copyFormattingDesc)(CFTypeRef cf);
} CFRuntimeClass;

CFTypeID _CFRuntimeRegisterClass(const CFRuntimeClass *cls);
const CFRuntimeClass *_CFRuntimeGetClass(CFTypeID typeID);
CFTypeRef _CFRuntimeCreateInstance(CFAllocatorRef allocator, CFTypeID typeID, CFIndex extraBytes);
void _CFRuntimeInitStaticInstance(void *memory, CFTypeID typeID);

#endif /* COREFOUNDATION_CFINTERNAL_H */
