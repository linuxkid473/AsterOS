/* Copyright (c) 2026 Vihaan Nathan -- see NSNotification.h */
#import <Foundation/Foundation.h>
#include <objc/message.h>

@implementation NSNotification
{
	NSString *_name;
	id _object;
	NSDictionary *_userInfo;
}

+ (instancetype)notificationWithName:(NSString *)name object:(id)object userInfo:(NSDictionary *)userInfo
{
	NSNotification *n = [[self alloc] init];
	n->_name = [name retain];
	n->_object = [object retain];
	n->_userInfo = [userInfo retain];
	return [n autorelease];
}

- (void)dealloc
{
	[_name release];
	[_object release];
	[_userInfo release];
	[super dealloc];
}

- (NSString *)name { return _name; }
- (id)object { return _object; }
- (NSDictionary *)userInfo { return _userInfo; }

- (id)copyWithZone:(NSZone *)zone
{
	(void)zone;
	return [self retain];
}

@end

/* Private, unexported entry type -- not declared in any public header.
 * `_observer` is deliberately not retained (see NSNotification.h's
 * header comment on real NSNotificationCenter's non-owning contract). */
@interface _NSNotificationObserverEntry : NSObject
{
	@public
	id _observer;
	SEL _selector;
	NSString *_name;	/* nil = any name */
	id _object;		/* unretained, nil = any object */
}
@end

@implementation _NSNotificationObserverEntry

- (void)dealloc
{
	[_name release];
	[super dealloc];
}

@end

@implementation NSNotificationCenter
{
	NSMutableArray *_entries;
}

+ (instancetype)defaultCenter
{
	static NSNotificationCenter *shared;
	if (!shared) {
		shared = [[self alloc] init];
	}
	return shared;
}

- (instancetype)init
{
	self = [super init];
	if (self) {
		_entries = [[NSMutableArray alloc] init];
	}
	return self;
}

- (void)dealloc
{
	[_entries release];
	[super dealloc];
}

- (void)addObserver:(id)observer selector:(SEL)selector name:(NSString *)name object:(id)object
{
	_NSNotificationObserverEntry *e = [[_NSNotificationObserverEntry alloc] init];
	e->_observer = observer;
	e->_selector = selector;
	e->_name = [name retain];
	e->_object = object;
	[_entries addObject:e];
	[e release];
}

- (void)removeObserver:(id)observer
{
	[self removeObserver:observer name:nil object:nil];
}

- (void)removeObserver:(id)observer name:(NSString *)name object:(id)object
{
	for (NSUInteger i = [_entries count]; i > 0; i--) {
		_NSNotificationObserverEntry *e = [_entries objectAtIndex:i - 1];
		if (e->_observer != observer) {
			continue;
		}
		if (name && (!e->_name || ![e->_name isEqualToString:name])) {
			continue;
		}
		if (object && e->_object != object) {
			continue;
		}
		[_entries removeObjectAtIndex:i - 1];
	}
}

- (void)postNotification:(NSNotification *)notification
{
	NSString *name = [notification name];
	id object = [notification object];
	/* Copy the entry list before dispatching: an observer's handler is
	 * free to add/remove observers (including itself), and mutating
	 * _entries while iterating it would be a real, if rare, correctness
	 * bug -- not hypothetical, this is exactly the kind of thing a
	 * -dealloc-triggered -removeObserver: does mid-notification. */
	NSArray *snapshot = [[_entries copy] autorelease];
	NSUInteger count = [snapshot count];
	for (NSUInteger i = 0; i < count; i++) {
		_NSNotificationObserverEntry *e = [snapshot objectAtIndex:i];
		if (e->_name && ![e->_name isEqualToString:name]) {
			continue;
		}
		if (e->_object && e->_object != object) {
			continue;
		}
		((void (*)(id, SEL, id))objc_msgSend)(e->_observer, e->_selector, notification);
	}
}

- (void)postNotificationName:(NSString *)name object:(id)object
{
	[self postNotificationName:name object:object userInfo:nil];
}

- (void)postNotificationName:(NSString *)name object:(id)object userInfo:(NSDictionary *)userInfo
{
	[self postNotification:[NSNotification notificationWithName:name object:object userInfo:userInfo]];
}

@end
