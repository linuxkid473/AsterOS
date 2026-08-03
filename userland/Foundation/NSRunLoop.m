/* Copyright (c) 2026 Vihaan Nathan -- see NSRunLoop.h */
#import <Foundation/Foundation.h>
#include <time.h>

/* Not `const` here -- see NSException.m's header comment on this exact
 * pattern. */
NSString *NSDefaultRunLoopMode = (NSString *)0;

__attribute__((constructor(200)))
static void
initRunLoopModeConstant(void)
{
	NSDefaultRunLoopMode = [[NSString stringWithUTF8String:"kCFRunLoopDefaultMode"] retain];
}

@implementation NSRunLoop
{
	NSMutableArray *_timers;
}

+ (instancetype)currentRunLoop
{
	/* Single global run loop, not per-thread -- same documented
	 * simplification as libobjc's autorelease pool stack (see
	 * userland/libobjc/autorelease.c), for the same reason: nothing in
	 * this tree drives a run loop from more than one thread yet. */
	static NSRunLoop *shared;
	if (!shared) {
		shared = [[self alloc] init];
	}
	return shared;
}

+ (instancetype)mainRunLoop
{
	return [self currentRunLoop];
}

- (instancetype)init
{
	self = [super init];
	if (self) {
		_timers = [[NSMutableArray alloc] init];
	}
	return self;
}

- (void)dealloc
{
	[_timers release];
	[super dealloc];
}

- (void)addTimer:(NSTimer *)timer forMode:(NSString *)mode
{
	(void)mode;	/* single mode only -- see header comment */
	[_timers addObject:timer];
}

- (void)purgeInvalidTimers
{
	for (NSUInteger i = [_timers count]; i > 0; i--) {
		if (![[_timers objectAtIndex:i - 1] isValid]) {
			[_timers removeObjectAtIndex:i - 1];
		}
	}
}

- (BOOL)runMode:(NSString *)mode beforeDate:(NSDate *)limitDate
{
	(void)mode;
	[self purgeInvalidTimers];

	NSTimer *earliest = nil;
	NSUInteger count = [_timers count];
	for (NSUInteger i = 0; i < count; i++) {
		NSTimer *t = [_timers objectAtIndex:i];
		if (!earliest || [[t fireDate] compare:[earliest fireDate]] == NSOrderedAscending) {
			earliest = t;
		}
	}
	if (!earliest) {
		return NO;	/* nothing scheduled */
	}

	NSTimeInterval wait = [[earliest fireDate] timeIntervalSinceNow];
	NSTimeInterval maxWait = [limitDate timeIntervalSinceNow];
	if (wait > maxWait) {
		wait = maxWait;
	}
	if (wait > 0) {
		struct timespec ts;
		ts.tv_sec = (time_t)wait;
		ts.tv_nsec = (long)((wait - (double)ts.tv_sec) * 1000000000.0);
		nanosleep(&ts, NULL);
	}
	if ([[NSDate date] compare:[earliest fireDate]] != NSOrderedAscending) {
		[earliest fire];
	}
	return YES;
}

- (void)runUntilDate:(NSDate *)limitDate
{
	while ([[NSDate date] compare:limitDate] == NSOrderedAscending) {
		if (![self runMode:NSDefaultRunLoopMode beforeDate:limitDate]) {
			break;	/* nothing scheduled left to wait for */
		}
	}
	[self purgeInvalidTimers];
}

@end
