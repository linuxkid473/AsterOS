/* Copyright (c) 2026 Vihaan Nathan -- see NSException.h
 *
 * Compiled without ARC (-fno-objc-arc, see build.sh): this class manages
 * its own ivar retain/release manually, same reasoning as every other
 * non-cluster class in this tree. Ivars declared directly in the
 * @implementation block (nonfragile ABI2 supports this -- keeps them out
 * of the public header, matching real Foundation's @private convention
 * without needing a class-continuation category).
 */
#import <Foundation/Foundation.h>
#include <stdio.h>
#include <stdlib.h>

/* Not `const` -- see NSException.h's header comment on why (real
 * objects built once by the constructor below, not `@"literal"`
 * compile-time constants). Originally these WERE declared `const` with
 * the constructor mutating them through a cast-const pointer; that
 * genuinely broke cross-image (write-through-cast-const-pointer is UB,
 * and something in this optimization pipeline exploited it) --
 * foundationtest, a separate executable linking this dylib, kept
 * observing the pre-constructor NULL value even after the constructor
 * had definitely already run and wrote the real value, caught live in
 * QEMU via a debug dump showing NSInvalidArgumentException=0x0. Making
 * the storage genuinely non-const (both here and in the header) fixed
 * it outright -- a real object, not a string literal, see NSString.m's
 * header comment on why `@"..."` isn't supported. */
NSString *NSGenericException = (NSString *)0;
NSString *NSInvalidArgumentException = (NSString *)0;
NSString *NSRangeException = (NSString *)0;
NSString *NSInternalInconsistencyException = (NSString *)0;
NSString *NSMallocException = (NSString *)0;

__attribute__((constructor(200)))
static void
initExceptionNameConstants(void)
{
	/* Priority 200 (CF's own singleton constructors, unprioritized, run
	 * at the default 65535; explicit lower numbers run earlier) so
	 * these are valid before any other Foundation constructor could
	 * plausibly raise. */
	NSGenericException = [[NSString stringWithUTF8String:"NSGenericException"] retain];
	NSInvalidArgumentException = [[NSString stringWithUTF8String:"NSInvalidArgumentException"] retain];
	NSRangeException = [[NSString stringWithUTF8String:"NSRangeException"] retain];
	NSInternalInconsistencyException = [[NSString stringWithUTF8String:"NSInternalInconsistencyException"] retain];
	NSMallocException = [[NSString stringWithUTF8String:"NSMallocException"] retain];
}

static NSHandler2 *g_handlerTop;

void
_NSPushHandler(NSHandler2 *h)
{
	h->next = g_handlerTop;
	h->exception = nil;
	g_handlerTop = h;
}

void
_NSPopHandlerIfCurrent(NSHandler2 *h)
{
	if (g_handlerTop == h) {
		g_handlerTop = h->next;
	}
}

void
_NSExceptionRaise(NSException *e)
{
	NSHandler2 *h = g_handlerTop;
	if (!h) {
		fprintf(stderr, "*** Terminating app due to uncaught exception '%s', reason: '%s'\n",
			[[e name] UTF8String] ? [[e name] UTF8String] : "(null)",
			[[e reason] UTF8String] ? [[e reason] UTF8String] : "(null)");
		abort();
	}
	g_handlerTop = h->next;
	/* +1 retain, then autorelease -- keeps `e` valid at least through the
	 * current autorelease pool's lifetime without requiring NS_HANDLER's
	 * body to release `localException` itself, matching real Cocoa's
	 * "caught exceptions are autoreleased" convention. */
	h->exception = [[e retain] autorelease];
	longjmp(h->jumpState, 1);
}

@implementation NSException
{
	NSString *_name;
	NSString *_reason;
	NSDictionary *_userInfo;
}

+ (instancetype)exceptionWithName:(NSString *)name reason:(NSString *)reason userInfo:(NSDictionary *)userInfo
{
	return [[[self alloc] initWithName:name reason:reason userInfo:userInfo] autorelease];
}

+ (void)raise:(NSString *)name format:(NSString *)format, ...
{
	va_list args;
	va_start(args, format);
	CFStringRef reason = CFStringCreateWithFormatAndArguments(kCFAllocatorDefault, NULL, (CFStringRef)format, args);
	va_end(args);
	NSException *e = [self exceptionWithName:name reason:(id)reason userInfo:nil];
	CFRelease(reason);
	[e raise];
}

- (instancetype)initWithName:(NSString *)name reason:(NSString *)reason userInfo:(NSDictionary *)userInfo
{
	self = [super init];
	if (self) {
		_name = [name retain];
		_reason = [reason retain];
		_userInfo = [userInfo retain];
	}
	return self;
}

- (void)dealloc
{
	[_name release];
	[_reason release];
	[_userInfo release];
	[super dealloc];
}

- (NSString *)name { return _name; }
- (NSString *)reason { return _reason; }
- (NSDictionary *)userInfo { return _userInfo; }

- (NSString *)description
{
	CFStringRef fmt = CFStringCreateWithCString(kCFAllocatorDefault, "%@: %@", kCFStringEncodingUTF8);
	CFStringRef desc = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, fmt, _name, _reason);
	CFRelease(fmt);
	return [(id)desc autorelease];
}

- (void)raise
{
	_NSExceptionRaise(self);
}

- (id)copyWithZone:(NSZone *)zone
{
	(void)zone;
	return [self retain];
}

@end
