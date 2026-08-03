/* Copyright (c) 2026 Vihaan Nathan
 *
 * Real, working exception propagation via NS_DURING/NS_HANDLER/
 * NS_ENDHANDLER -- genuine historical Foundation API (it predates the
 * modern zero-cost @try/@catch/@throw compiler feature, and still ships
 * in real Foundation's NSException.h today as a deprecated-but-present
 * mechanism), implemented here with this tree's own real setjmp/longjmp
 * (userland/libc).
 *
 * Modern `@try { } @catch (NSException *e) { } @throw e;` Objective-C
 * SYNTAX is NOT supported: clang compiles it unconditionally to
 * zero-cost DWARF unwinding (calls to _objc_exception_throw / the
 * __objc_personality_v0 personality routine / _Unwind_RaiseException),
 * confirmed empirically -- this host clang has no
 * `-fobjc-sjlj-exceptions` fallback for x86_64 (`clang: error: unknown
 * argument`), and this project has no DWARF/.eh_frame unwinder yet (a
 * subsystem on the scale of another full phase, tracked separately, not
 * part of Foundation). Use NS_DURING/NS_HANDLER/NS_ENDHANDLER instead --
 * see docs/architecture.md.
 *
 * Single global handler stack, not per-thread -- same documented
 * simplification as userland/libobjc/autorelease.c's autorelease pool
 * stack, for the same reason (nothing in this tree exercises concurrent
 * exception handling across real threads yet).
 */
#ifndef FOUNDATION_NSEXCEPTION_H
#define FOUNDATION_NSEXCEPTION_H

#include <Foundation/NSObject.h>
#include <setjmp.h>

#ifdef __cplusplus
extern "C" {
#endif

@class NSDictionary;

@interface NSException : NSObject <NSCopying, NSCoding>

+ (instancetype)exceptionWithName:(NSString *)name reason:(NSString *)reason userInfo:(NSDictionary *)userInfo;
+ (void)raise:(NSString *)name format:(NSString *)format, ...;

- (instancetype)initWithName:(NSString *)name reason:(NSString *)reason userInfo:(NSDictionary *)userInfo;

- (NSString *)name;
- (NSString *)reason;
- (NSDictionary *)userInfo;

- (void)raise;

@end

/* Real Apple types these `NSString * const` -- safe there because
 * `@"literal"` compile-time string constants need no runtime
 * initialization at all. This tree doesn't support `@"literal"` syntax
 * (see NSString.m's header comment: it would need replicating a
 * genuinely fragile piece of the ObjC ABI), so these are real objects
 * built once by a constructor instead -- which means the storage can't
 * be `const` here (see NSException.m's header comment for the actual
 * cross-image bug hit when it was). Read-only in practice; nothing in
 * this tree ever reassigns them. */
FOUNDATION_EXPORT NSString *NSGenericException;
FOUNDATION_EXPORT NSString *NSInvalidArgumentException;
FOUNDATION_EXPORT NSString *NSRangeException;
FOUNDATION_EXPORT NSString *NSInternalInconsistencyException;
FOUNDATION_EXPORT NSString *NSMallocException;

/* ---- NS_DURING/NS_HANDLER/NS_ENDHANDLER internals ----
 * Not part of the public API surface -- real client code only ever
 * touches the macros below, never these directly (same convention as
 * real Foundation's NSException.h). */
typedef struct _NSHandler2 {
	jmp_buf jumpState;
	struct _NSHandler2 *next;
	/* __unsafe_unretained, deliberately: this field is manually managed
	 * (_NSExceptionRaise retains-then-autoreleases before the longjmp,
	 * see NSException.m) rather than ARC-owned. Without this explicit
	 * annotation, an ARC-compiled translation unit that expands
	 * NS_DURING (declaring one of these as a local) infers a __strong
	 * field and emits exception-safety cleanup landing pads for it --
	 * real code, but calling into `___objc_personality_v0`/
	 * `_Unwind_Resume`, which this project has no unwinder to back (see
	 * this header's own top comment). Confirmed empirically: dropping
	 * this annotation reproduces exactly those two undefined symbols at
	 * link time in any ARC-compiled client of NS_DURING. */
	__unsafe_unretained NSException *exception;
} NSHandler2;

FOUNDATION_EXPORT void _NSPushHandler(NSHandler2 *h);
FOUNDATION_EXPORT void _NSPopHandlerIfCurrent(NSHandler2 *h);
FOUNDATION_EXPORT void _NSExceptionRaise(NSException *e);

#define NS_DURING \
	{ \
		NSHandler2 _nsHandler; \
		_NSPushHandler(&_nsHandler); \
		if (setjmp(_nsHandler.jumpState) == 0) {

#define NS_HANDLER \
		} else {

#define NS_ENDHANDLER \
		} \
		_NSPopHandlerIfCurrent(&_nsHandler); \
	}

#define localException (_nsHandler.exception)

#ifdef __cplusplus
}
#endif

#endif /* FOUNDATION_NSEXCEPTION_H */
