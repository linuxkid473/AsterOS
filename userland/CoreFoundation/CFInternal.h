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
 *
 * `isa`: real toll-free bridging support (Foundation, see
 * userland/Foundation/). This is a plain `void *`, deliberately laid out
 * as the literal first field so it lines up byte-for-byte with libobjc's
 * `struct objc_object { Class isa; }` (userland/libobjc/objc_priv.h) --
 * a bridged CFTypeRef cast to `id` is a real, dispatchable Objective-C
 * object, not a lookalike. CF itself never reads or writes this field
 * except to set it from the bridge-class table below at instance-creation
 * time; nothing here message-sends anything, so CF stays link-independent
 * of libobjc (see build.sh's own comment) even though the field exists.
 * Unlike real CF's `_cfisa` (which tag-encodes the CFTypeID itself), this
 * tree keeps CFTypeID as its own explicit field below -- one word bigger,
 * but no decode logic, consistent with every other "real semantics,
 * simpler storage" tradeoff in this file.
 */
#ifndef COREFOUNDATION_CFINTERNAL_H
#define COREFOUNDATION_CFINTERNAL_H

#include <CoreFoundation/CoreFoundation.h>

#define CF_MAX_RUNTIME_CLASSES 32

typedef struct __CFRuntimeBase {
	void *isa;		/* NULL until Foundation calls _CFRuntimeBridgeClasses for this typeID; objc_object-layout-compatible */
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

/* Toll-free bridging registration: Foundation calls this once per bridged
 * CF/NS pair (e.g. CFStringGetTypeID(), &_OBJC_CLASS_$_NSCFString) at load
 * time, before creating any Foundation-facing objects. Every instance of
 * `typeID` created afterward (via _CFRuntimeCreateInstance/
 * _CFRuntimeInitStaticInstance) gets `isaClass` as its isa. Safe to call
 * before any instances of that type exist -- CF programs that never link
 * Foundation simply never call this, and every instance's isa stays NULL,
 * which nothing in pure-CF code ever dereferences. */
void _CFRuntimeBridgeClasses(CFTypeID typeID, void *isaClass);
void *_CFRuntimeGetBridgedClass(CFTypeID typeID);

/* Three CF singletons (kCFNull, kCFBooleanTrue, kCFBooleanFalse --
 * CFNull.c/CFBoolean.c) self-register via __attribute__((constructor))
 * instead of the usual pthread_once-on-first-call pattern (see their own
 * header comments), specifically so client code can dereference them
 * before calling any other CF entry point. That means their isa is
 * always set from an empty bridge table -- dyld runs mod-init-funcs in
 * image dependency order, so CF's constructors unconditionally run
 * before Foundation's own (Foundation depends on CF, never the reverse)
 * -- _CFRuntimeBridgeClasses can never reach them retroactively. This is
 * the escape hatch: Foundation calls it directly, by pointer, on exactly
 * those three known statically-allocated instances at its own init time. */
void _CFRuntimeSetInstanceISA(CFTypeRef cf, void *isaClass);

#endif /* COREFOUNDATION_CFINTERNAL_H */
