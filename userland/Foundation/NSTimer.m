/* Copyright (c) 2026 Vihaan Nathan -- see NSTimer.h */
#import <Foundation/Foundation.h>
#include <objc/message.h>

@implementation NSTimer
{
	@public
	NSDate *_fireDate;
	NSTimeInterval _interval;
	id _target;
	SEL _selector;
	id _userInfo;
	BOOL _repeats;
	BOOL _valid;
}

+ (instancetype)timerWithTimeInterval:(NSTimeInterval)interval target:(id)target selector:(SEL)selector userInfo:(id)userInfo repeats:(BOOL)repeats
{
	NSTimer *t = [[self alloc] init];
	t->_fireDate = [[NSDate dateWithTimeIntervalSinceNow:interval] retain];
	t->_interval = interval;
	t->_target = [target retain];
	t->_selector = selector;
	t->_userInfo = [userInfo retain];
	t->_repeats = repeats;
	t->_valid = YES;
	return [t autorelease];
}

+ (instancetype)scheduledTimerWithTimeInterval:(NSTimeInterval)interval target:(id)target selector:(SEL)selector userInfo:(id)userInfo repeats:(BOOL)repeats
{
	NSTimer *t = [self timerWithTimeInterval:interval target:target selector:selector userInfo:userInfo repeats:repeats];
	[[NSRunLoop currentRunLoop] addTimer:t forMode:NSDefaultRunLoopMode];
	return t;
}

- (void)dealloc
{
	[_fireDate release];
	[_target release];
	[_userInfo release];
	[super dealloc];
}

- (void)fire
{
	if (!_valid) {
		return;
	}
	((void (*)(id, SEL, id))objc_msgSend)(_target, _selector, self);
	if (_repeats && _valid) {
		[_fireDate release];
		_fireDate = [[NSDate dateWithTimeIntervalSinceNow:_interval] retain];
	} else {
		[self invalidate];
	}
}

- (void)invalidate
{
	_valid = NO;
}

- (BOOL)isValid { return _valid; }
- (NSDate *)fireDate { return _fireDate; }
- (NSTimeInterval)timeInterval { return _interval; }
- (id)userInfo { return _userInfo; }

@end
