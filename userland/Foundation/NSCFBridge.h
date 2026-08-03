/* Copyright (c) 2026 Vihaan Nathan
 *
 * Shared helpers every toll-free-bridged NSCFFoo class (NSCFString,
 * NSCFArray, NSCFDictionary, NSCFSet, NSCFData, NSCFNumber, NSCFNull)
 * wires its -retain/-release/-retainCount/-isEqual:/-hash/-dealloc
 * through. This is what makes bridging "toll-free" rather than a copy:
 * a bridged object's retain count and equality are answered by CF
 * itself, identically whether the caller went through CFRetain/CFEqual
 * or -retain/-isEqual:. Not a shared base class -- Objective-C has no
 * mixins, and each NSCFFoo already has a real, distinct Foundation
 * superclass (NSString, NSArray, ...) -- so this is instead a handful
 * of one-line C helpers each @implementation calls into directly.
 *
 * -dealloc is intentionally never reachable in correct usage: CFRelease
 * already frees the object itself (via the CFRuntimeClass finalize
 * callback) the moment a bridged object's retain count hits zero, so
 * nothing ever message-sends -dealloc on one. NSCFBridge_deallocGuard
 * exists only to make a stray direct `[obj dealloc]` call loud instead
 * of double-freeing.
 */
#ifndef FOUNDATION_NSCFBRIDGE_H
#define FOUNDATION_NSCFBRIDGE_H

#include <objc/objc.h>
#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/NSObjCRuntime.h>

@class NSString;

id NSCFBridge_retain(id self);
void NSCFBridge_release(id self);
NSUInteger NSCFBridge_retainCount(id self);
BOOL NSCFBridge_isEqual(id self, id other);
NSUInteger NSCFBridge_hash(id self);
NSString *NSCFBridge_description(id self);
void NSCFBridge_deallocGuard(id self);

/* ---- collection element callbacks ----
 *
 * NSArray/NSDictionary/NSSet must NOT use CF's own kCFType*CallBacks
 * (CFRetain/CFRelease/CFEqual/CFHash/CFCopyDescription) for their
 * element storage: those assume every element has a real CFRuntimeBase
 * layout (isa/typeID/isConstant/retainCount), which only holds for
 * toll-free-bridged objects (NSString/NSNumber/NSArray/...). A plain
 * Objective-C object -- any ordinary NSObject subclass, allocated via
 * the normal _objc_rootAlloc path -- has none of those trailing fields,
 * so CFRetain reading/writing at those offsets is memory corruption.
 * Caught live in QEMU as a silent hard crash (no CHECK failure printed,
 * just a process that stopped producing any output at all) the moment
 * NSNotificationCenter's own observer-entry array -- holding plain
 * internal NSObject instances, not bridged ones -- was actually
 * exercised.
 *
 * These callbacks instead go through real objc_msgSend
 * (-retain/-release/-isEqual:/-hash/-description), the same thing real
 * NSArray/NSDictionary/NSSet do: correct for a toll-free-bridged
 * element too, since -retain/-release on one of *those* already
 * forwards to CFRetain/CFRelease (see NSCFBridge_retain/release above)
 * -- and correct for a plain object, which only ever understands
 * Objective-C messages to begin with. Dictionary *keys* specifically
 * use -copy, not -retain, matching real NSDictionary's NSCopying
 * contract for keys. */
extern const CFArrayCallBacks kNSObjectArrayCallBacks;
extern const CFDictionaryKeyCallBacks kNSObjectDictionaryKeyCallBacks;
extern const CFDictionaryValueCallBacks kNSObjectDictionaryValueCallBacks;
extern const CFSetCallBacks kNSObjectSetCallBacks;

/* Formats a double as decimal text into `out` (at least 32 bytes),
 * without ever going through CFStringCreateWithFormat's "%g"/"%f"/"%e" --
 * this tree's libc vsnprintf has NO floating-point conversion support at
 * all (documented in userland/CoreFoundation/CFString.c's own header
 * comment: those conversions silently fall through to a default case
 * that neither prints the value nor consumes the va_arg, desyncing
 * every argument after it). Caught live in QEMU: NSPropertyListSerialization
 * and NSJSONSerialization's real-number writers both used to do exactly
 * this and produced garbage. Not a shortest-round-trip dtoa -- up to 15
 * fractional digits, trailing zeros trimmed -- but numerically correct,
 * which is all either format needs. */
void NSCFBridge_formatDouble(double d, char *out, size_t outsize);

#endif /* FOUNDATION_NSCFBRIDGE_H */
