/* Copyright (c) 2026 Vihaan Nathan
 *
 * The root class. NSObject is its own root (__attribute__((objc_root_class)),
 * see NSObject.m) rather than a subclass of libobjc's existing bare `Object`
 * root (userland/libobjc/Root.m) -- that class stays untouched, it's
 * objctest's own ground-truth regression target and predates Foundation's
 * existence entirely (see its header comment). Two independent root
 * classes coexisting is period-accurate Objective-C history, not a
 * shortcut: NeXT's old `Object` and NSObject genuinely coexisted for
 * years on real Darwin. NSObject.m calls the exact same _objc_root*
 * C helpers (arc.c/autorelease.c) Root.m does.
 */
#ifndef FOUNDATION_NSOBJECT_H
#define FOUNDATION_NSOBJECT_H

#include <Foundation/NSObjCRuntime.h>

#ifdef __cplusplus
extern "C" {
#endif

@class NSString;
@class NSCoder;
@class NSMethodSignature;
@class NSInvocation;

@protocol NSObject

- (BOOL)isEqual:(id)object;
- (NSUInteger)hash;

- (Class)superclass;
- (Class)class;
- (id)self;

- (BOOL)isKindOfClass:(Class)aClass;
- (BOOL)isMemberOfClass:(Class)aClass;
- (BOOL)conformsToProtocol:(Protocol *)aProtocol;
- (BOOL)respondsToSelector:(SEL)aSelector;

- (id)retain;
- (oneway void)release;
- (id)autorelease;
- (NSUInteger)retainCount;

- (NSString *)description;

@end

@protocol NSCopying
- (id)copyWithZone:(NSZone *)zone;
@end

@protocol NSMutableCopying
- (id)mutableCopyWithZone:(NSZone *)zone;
@end

@protocol NSCoding
- (void)encodeWithCoder:(NSCoder *)coder;
- (id)initWithCoder:(NSCoder *)coder;
@end

__attribute__((objc_root_class))
@interface NSObject <NSObject>
{
	Class isa;
}

+ (id)alloc;
+ (id)new;
+ (Class)class;
+ (Class)superclass;
+ (BOOL)isSubclassOfClass:(Class)aClass;
+ (BOOL)instancesRespondToSelector:(SEL)aSelector;
+ (BOOL)respondsToSelector:(SEL)aSelector;
+ (BOOL)conformsToProtocol:(Protocol *)aProtocol;
+ (NSString *)description;

- (id)init;
- (void)dealloc;

- (id)copy;
- (id)mutableCopy;

- (id)performSelector:(SEL)aSelector;
- (id)performSelector:(SEL)aSelector withObject:(id)object;
- (id)performSelector:(SEL)aSelector withObject:(id)object1 withObject:(id)object2;

- (void)doesNotRecognizeSelector:(SEL)aSelector;

@end

#ifdef __cplusplus
}
#endif

#endif /* FOUNDATION_NSOBJECT_H */
