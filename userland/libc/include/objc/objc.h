/* Copyright (c) 2026 Vihaan Nathan
 *
 * Base Objective-C types. id/SEL/Class/IMP are compiler builtins when
 * clang is in Objective-C mode (-fobjc-runtime=...), but plain C
 * translation units (the runtime's own implementation included) need
 * real typedefs -- this header provides those, matching Apple's public
 * objc/objc.h closely enough for real .m source to #import unmodified.
 */
#ifndef _OBJC_OBJC_H_
#define _OBJC_OBJC_H_

#include <stddef.h>

#if !__OBJC__
typedef struct objc_class *Class;
typedef struct objc_object {
	Class isa;
} *id;
#endif

typedef struct objc_selector *SEL;
typedef id (*IMP)(id, SEL, ...);

typedef signed char BOOL;
#define YES ((BOOL)1)
#define NO ((BOOL)0)

#ifndef Nil
#define Nil ((Class)0)
#endif
#ifndef nil
#define nil ((id)0)
#endif

typedef struct objc_ivar *Ivar;
typedef struct objc_method *Method;
typedef struct objc_category *Category;
typedef struct objc_property *objc_property_t;

#if __OBJC__
@class Protocol;
#else
typedef struct objc_object Protocol;
#endif

const char *sel_getName(SEL sel);
BOOL sel_isEqual(SEL lhs, SEL rhs);

#endif /* _OBJC_OBJC_H_ */
