/* Copyright (c) 2026 Vihaan Nathan -- see NSError.h */
#import <Foundation/Foundation.h>

/* Not `const` -- see NSException.m's header comment on this exact
 * pattern (a real object built once by a constructor, not a
 * `@"literal"`; a `const`-qualified version of this genuinely broke
 * cross-image reads, ground-truthed live in QEMU). */
NSString *NSCocoaErrorDomain = (NSString *)0;
NSString *NSPOSIXErrorDomain = (NSString *)0;
NSString *NSLocalizedDescriptionKey = (NSString *)0;

__attribute__((constructor(200)))
static void
initErrorConstants(void)
{
	NSCocoaErrorDomain = [[NSString stringWithUTF8String:"NSCocoaErrorDomain"] retain];
	NSPOSIXErrorDomain = [[NSString stringWithUTF8String:"NSPOSIXErrorDomain"] retain];
	NSLocalizedDescriptionKey = [[NSString stringWithUTF8String:"NSLocalizedDescriptionKey"] retain];
}

@implementation NSError
{
	NSString *_domain;
	NSInteger _code;
	NSDictionary *_userInfo;
}

+ (instancetype)errorWithDomain:(NSString *)domain code:(NSInteger)code userInfo:(NSDictionary *)dict
{
	return [[[self alloc] initWithDomain:domain code:code userInfo:dict] autorelease];
}

- (instancetype)initWithDomain:(NSString *)domain code:(NSInteger)code userInfo:(NSDictionary *)dict
{
	self = [super init];
	if (self) {
		_domain = [domain retain];
		_code = code;
		_userInfo = [dict retain];
	}
	return self;
}

- (void)dealloc
{
	[_domain release];
	[_userInfo release];
	[super dealloc];
}

- (NSString *)domain { return _domain; }
- (NSInteger)code { return _code; }
- (NSDictionary *)userInfo { return _userInfo; }

- (NSString *)localizedDescription
{
	id desc = [_userInfo objectForKey:NSLocalizedDescriptionKey];
	if (desc) {
		return desc;
	}
	return [NSString stringWithFormat:[NSString stringWithUTF8String:"The operation could not be completed. (%@ error %ld.)"], _domain, (long)_code];
}

- (NSString *)description
{
	return [self localizedDescription];
}

- (id)copyWithZone:(NSZone *)zone
{
	(void)zone;
	return [[NSError alloc] initWithDomain:_domain code:_code userInfo:_userInfo];
}

@end
