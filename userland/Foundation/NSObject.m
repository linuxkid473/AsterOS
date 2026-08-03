/* Copyright (c) 2026 Vihaan Nathan
 *
 * NSObject: real Objective-C compiled through the same path as every
 * other class here (see NSObject.h's header comment for why this is a
 * second, independent root class alongside libobjc's `Object`).
 * Compiled without ARC (-fno-objc-arc, see build.sh) -- same reasoning
 * as Root.m: this class defines what retain/release/dealloc mean.
 */
#include <objc/objc.h>
#include <objc/runtime.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

extern id _objc_rootAlloc(Class cls);
extern id _objc_rootInit(id obj);
extern void _objc_rootDealloc(id obj);
extern id _objc_rootRetain(id obj);
extern void _objc_rootRelease(id obj);
extern id _objc_rootAutorelease(id obj);
extern uintptr_t _objc_rootRetainCount(id obj);
extern Class _objc_rootClass(id obj);
extern BOOL _objc_rootIsKindOfClass(id obj, Class cls);
extern BOOL _objc_rootRespondsToSelector(id obj, SEL sel);

#import <Foundation/Foundation.h>

@implementation NSObject

+ (id)alloc
{
	return _objc_rootAlloc(self);
}

+ (id)new
{
	return [[self alloc] init];
}

+ (Class)class
{
	return self;
}

+ (Class)superclass
{
	return class_getSuperclass(self);
}

+ (BOOL)isSubclassOfClass:(Class)aClass
{
	for (Class c = self; c; c = class_getSuperclass(c)) {
		if (c == aClass) {
			return YES;
		}
	}
	return NO;
}

+ (BOOL)instancesRespondToSelector:(SEL)aSelector
{
	return class_respondsToSelector(self, aSelector);
}

+ (BOOL)respondsToSelector:(SEL)aSelector
{
	return class_respondsToSelector(object_getClass((id)self), aSelector);
}

+ (BOOL)conformsToProtocol:(Protocol *)aProtocol
{
	return class_conformsToProtocol(self, aProtocol);
}

+ (NSString *)description
{
	return [NSString stringWithUTF8String:class_getName(self)];
}

- (id)init
{
	return _objc_rootInit(self);
}

- (void)dealloc
{
	_objc_rootDealloc(self);
}

- (id)retain
{
	return _objc_rootRetain(self);
}

- (oneway void)release
{
	_objc_rootRelease(self);
}

- (id)autorelease
{
	return _objc_rootAutorelease(self);
}

- (NSUInteger)retainCount
{
	return (NSUInteger)_objc_rootRetainCount(self);
}

- (Class)class
{
	return _objc_rootClass(self);
}

- (Class)superclass
{
	return class_getSuperclass(_objc_rootClass(self));
}

- (id)self
{
	return self;
}

- (BOOL)isKindOfClass:(Class)aClass
{
	return _objc_rootIsKindOfClass(self, aClass);
}

- (BOOL)isMemberOfClass:(Class)aClass
{
	return _objc_rootClass(self) == aClass;
}

- (BOOL)conformsToProtocol:(Protocol *)aProtocol
{
	return class_conformsToProtocol(_objc_rootClass(self), aProtocol);
}

- (BOOL)respondsToSelector:(SEL)aSelector
{
	return _objc_rootRespondsToSelector(self, aSelector);
}

- (BOOL)isEqual:(id)other
{
	return self == other;
}

- (NSUInteger)hash
{
	return (NSUInteger)(uintptr_t)self;
}

- (NSString *)description
{
	return [NSString stringWithFormat:[NSString stringWithUTF8String:"<%s: %p>"], class_getName(_objc_rootClass(self)), self];
}

- (id)copy
{
	return [(id<NSCopying>)self copyWithZone:0];
}

- (id)mutableCopy
{
	return [(id<NSMutableCopying>)self mutableCopyWithZone:0];
}

- (id)performSelector:(SEL)aSelector
{
	return objc_msgSend(self, aSelector);
}

- (id)performSelector:(SEL)aSelector withObject:(id)object
{
	return ((id (*)(id, SEL, id))objc_msgSend)(self, aSelector, object);
}

- (id)performSelector:(SEL)aSelector withObject:(id)object1 withObject:(id)object2
{
	return ((id (*)(id, SEL, id, id))objc_msgSend)(self, aSelector, object1, object2);
}

- (void)doesNotRecognizeSelector:(SEL)aSelector
{
	[NSException raise:NSInvalidArgumentException
	            format:[NSString stringWithFormat:[NSString stringWithUTF8String:"%s: does not recognize selector %s"], class_getName(_objc_rootClass(self)), sel_getName(aSelector)]];
}

@end
