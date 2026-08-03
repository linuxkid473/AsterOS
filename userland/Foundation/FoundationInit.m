/* Copyright (c) 2026 Vihaan Nathan
 *
 * Toll-free bridging registration: runs once, before any other
 * Foundation code, via the same __DATA,__mod_init_func
 * (__attribute__((constructor))) mechanism CoreFoundation's own
 * self-registering singletons already use (see TODO.md's CoreFoundation
 * phase writeup) -- dyld runs mod-init-funcs in image dependency order,
 * so CF's own constructors (which register CFString/CFArray/etc.'s
 * CFTypeIDs) always run before this one (Foundation depends on CF,
 * never the reverse), guaranteeing every GetTypeID() call below is
 * live. _CFRuntimeBridgeClasses/_CFRuntimeSetInstanceISA are CF-private
 * SPI (userland/CoreFoundation/CFInternal.h) -- extern-declared here
 * directly rather than including that header, same convention
 * userland/libobjc/Root.m uses for the _objc_root* helpers.
 *
 * Every NSCFFoo class referenced here is otherwise undeclared anywhere
 * public (they're private concrete backing classes, see each Foo.m's
 * header comment) -- looked up by name via objc_getClass() rather than
 * needing an @interface visible in this translation unit.
 */
#include <CoreFoundation/CoreFoundation.h>
#include <objc/runtime.h>

extern void _CFRuntimeBridgeClasses(CFTypeID typeID, void *isaClass);
extern void _CFRuntimeSetInstanceISA(CFTypeRef cf, void *isaClass);

__attribute__((constructor))
static void
__FoundationInit(void)
{
	_CFRuntimeBridgeClasses(CFStringGetTypeID(), objc_getClass("NSCFString"));
	_CFRuntimeBridgeClasses(CFNullGetTypeID(), objc_getClass("NSCFNull"));
	_CFRuntimeBridgeClasses(CFBooleanGetTypeID(), objc_getClass("NSCFBoolean"));
	_CFRuntimeBridgeClasses(CFNumberGetTypeID(), objc_getClass("NSCFNumber"));
	_CFRuntimeBridgeClasses(CFArrayGetTypeID(), objc_getClass("NSCFArray"));
	_CFRuntimeBridgeClasses(CFDictionaryGetTypeID(), objc_getClass("NSCFDictionary"));
	_CFRuntimeBridgeClasses(CFSetGetTypeID(), objc_getClass("NSCFSet"));
	_CFRuntimeBridgeClasses(CFDataGetTypeID(), objc_getClass("NSCFData"));
	_CFRuntimeBridgeClasses(CFDateGetTypeID(), objc_getClass("NSCFDate"));
	_CFRuntimeBridgeClasses(CFTimeZoneGetTypeID(), objc_getClass("NSCFTimeZone"));
	_CFRuntimeBridgeClasses(CFLocaleGetTypeID(), objc_getClass("NSCFLocale"));
	_CFRuntimeBridgeClasses(CFURLGetTypeID(), objc_getClass("NSCFURL"));

	/* kCFNull/kCFBooleanTrue/kCFBooleanFalse are created by CF's own
	 * constructors before this one runs (see this file's header
	 * comment) -- _CFRuntimeBridgeClasses can't reach instances that
	 * already exist, so patch their isa directly. */
	_CFRuntimeSetInstanceISA(kCFNull, objc_getClass("NSCFNull"));
	_CFRuntimeSetInstanceISA(kCFBooleanTrue, objc_getClass("NSCFBoolean"));
	_CFRuntimeSetInstanceISA(kCFBooleanFalse, objc_getClass("NSCFBoolean"));
}
