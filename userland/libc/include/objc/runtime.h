/* Copyright (c) 2026 Vihaan Nathan
 *
 * Public runtime API surface -- signatures match Apple's objc/runtime.h
 * for the subset this runtime implements (see userland/libobjc/). Not
 * exhaustive: no method swizzling helpers, no associated objects, no
 * KVO-adjacent machinery -- see TODO.md Phase 13 for what's in scope.
 */
#ifndef _OBJC_RUNTIME_H_
#define _OBJC_RUNTIME_H_

#include <objc/objc.h>
#include <stdint.h>

/* Selectors */
SEL sel_registerName(const char *str);

/* Classes */
Class objc_getClass(const char *name);
const char *class_getName(Class cls);
Class class_getSuperclass(Class cls);
BOOL class_isMetaClass(Class cls);
size_t class_getInstanceSize(Class cls);
Ivar class_getInstanceVariable(Class cls, const char *name);
Ivar *class_copyIvarList(Class cls, unsigned int *outCount);
Method class_getInstanceMethod(Class cls, SEL name);
IMP class_getMethodImplementation(Class cls, SEL name);
BOOL class_respondsToSelector(Class cls, SEL sel);
BOOL class_conformsToProtocol(Class cls, Protocol *proto);
BOOL class_addMethod(Class cls, SEL name, IMP imp, const char *types);
BOOL class_addIvar(Class cls, const char *name, size_t size, uint8_t alignment, const char *types);
BOOL class_addProtocol(Class cls, Protocol *proto);
Class object_getClass(id obj);
Class object_setClass(id obj, Class cls);
const char *object_getClassName(id obj);

/* Ivar access */
const char *ivar_getName(Ivar ivar);
ptrdiff_t ivar_getOffset(Ivar ivar);
id object_getIvar(id obj, Ivar ivar);
void object_setIvar(id obj, Ivar ivar, id value);
Ivar object_setInstanceVariable(id obj, const char *name, void *value);
Ivar object_getInstanceVariable(id obj, const char *name, void **outValue);

/* Method introspection */
SEL method_getName(Method m);
IMP method_getImplementation(Method m);
const char *method_getTypeEncoding(Method m);

/* Dynamic class creation */
Class objc_allocateClassPair(Class superclass, const char *name, size_t extraBytes);
void objc_registerClassPair(Class cls);

/* Allocation */
id class_createInstance(Class cls, size_t extraBytes);
id objc_msgSend(id self, SEL op, ...); /* also in objc/message.h */

/* Image / metadata registration entry point dyld calls -- not part of
 * Apple's public API surface, but declared here since it's the one
 * exported symbol dyld looks up by name (see userland/dyld/dyld_main.c). */
struct mach_header_64;
void _objc_init(const struct mach_header_64 *const *mhs, int count);

#endif /* _OBJC_RUNTIME_H_ */
