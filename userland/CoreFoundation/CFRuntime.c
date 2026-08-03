/* Copyright (c) 2026 Vihaan Nathan */
#include "CFInternal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

static const CFRuntimeClass *g_classes[CF_MAX_RUNTIME_CLASSES];
static void *g_bridgeClasses[CF_MAX_RUNTIME_CLASSES];	/* parallel to g_classes, indexed the same way; NULL = not bridged */
static CFIndex g_classCount;
static pthread_mutex_t g_classLock = PTHREAD_MUTEX_INITIALIZER;

CFTypeID _CFRuntimeRegisterClass(const CFRuntimeClass *cls)
{
	pthread_mutex_lock(&g_classLock);
	CFTypeID typeID = (CFTypeID)(g_classCount + 1);	/* 0 stays "invalid", matching real CF */
	g_classes[g_classCount++] = cls;
	pthread_mutex_unlock(&g_classLock);
	return typeID;
}

const CFRuntimeClass *_CFRuntimeGetClass(CFTypeID typeID)
{
	if (typeID == 0 || typeID > (CFTypeID)g_classCount)
		return NULL;
	return g_classes[typeID - 1];
}

void _CFRuntimeBridgeClasses(CFTypeID typeID, void *isaClass)
{
	if (typeID == 0 || typeID > (CFTypeID)g_classCount)
		return;
	pthread_mutex_lock(&g_classLock);
	g_bridgeClasses[typeID - 1] = isaClass;
	pthread_mutex_unlock(&g_classLock);
}

void *_CFRuntimeGetBridgedClass(CFTypeID typeID)
{
	if (typeID == 0 || typeID > (CFTypeID)g_classCount)
		return NULL;
	return g_bridgeClasses[typeID - 1];
}

void _CFRuntimeSetInstanceISA(CFTypeRef cf, void *isaClass)
{
	((CFRuntimeBase *)cf)->isa = isaClass;
}

CFTypeRef _CFRuntimeCreateInstance(CFAllocatorRef allocator, CFTypeID typeID, CFIndex extraBytes)
{
	(void)allocator;	/* v1: every allocator is malloc-backed, see CFAllocator.c */
	CFRuntimeBase *base = calloc(1, sizeof(CFRuntimeBase) + (size_t)extraBytes);
	if (!base)
		return NULL;
	base->isa = _CFRuntimeGetBridgedClass(typeID);
	base->typeID = typeID;
	base->isConstant = false;
	base->retainCount = 1;
	return base;
}

void _CFRuntimeInitStaticInstance(void *memory, CFTypeID typeID)
{
	CFRuntimeBase *base = memory;
	base->isa = _CFRuntimeGetBridgedClass(typeID);
	base->typeID = typeID;
	base->isConstant = true;
	base->retainCount = 1;
}

CFTypeID CFGetTypeID(CFTypeRef cf)
{
	return ((const CFRuntimeBase *)cf)->typeID;
}

CFStringRef CFCopyTypeIDDescription(CFTypeID type_id)
{
	const CFRuntimeClass *cls = _CFRuntimeGetClass(type_id);
	return CFStringCreateWithCString(kCFAllocatorDefault, cls ? cls->className : "unknown", kCFStringEncodingUTF8);
}

CFTypeRef CFRetain(CFTypeRef cf)
{
	if (!cf)
		return NULL;
	CFRuntimeBase *base = (CFRuntimeBase *)cf;
	if (base->isConstant)
		return cf;
	__atomic_fetch_add(&base->retainCount, 1, __ATOMIC_RELAXED);
	return cf;
}

void CFRelease(CFTypeRef cf)
{
	if (!cf)
		return;
	CFRuntimeBase *base = (CFRuntimeBase *)cf;
	if (base->isConstant)
		return;
	if (__atomic_fetch_sub(&base->retainCount, 1, __ATOMIC_ACQ_REL) != 1)
		return;
	const CFRuntimeClass *cls = _CFRuntimeGetClass(base->typeID);
	if (cls && cls->finalize)
		cls->finalize(cf);
	free(base);
}

CFIndex CFGetRetainCount(CFTypeRef cf)
{
	const CFRuntimeBase *base = cf;
	if (base->isConstant)
		return 2147483647;	/* real CF returns a similarly large sentinel for constant objects */
	return __atomic_load_n(&base->retainCount, __ATOMIC_RELAXED);
}

Boolean CFEqual(CFTypeRef cf1, CFTypeRef cf2)
{
	if (cf1 == cf2)
		return true;
	if (!cf1 || !cf2)
		return false;
	const CFRuntimeBase *b1 = cf1, *b2 = cf2;
	if (b1->typeID != b2->typeID)
		return false;
	const CFRuntimeClass *cls = _CFRuntimeGetClass(b1->typeID);
	if (cls && cls->equal)
		return cls->equal(cf1, cf2);
	return false;
}

CFHashCode CFHash(CFTypeRef cf)
{
	const CFRuntimeBase *base = cf;
	const CFRuntimeClass *cls = _CFRuntimeGetClass(base->typeID);
	if (cls && cls->hash)
		return cls->hash(cf);
	return (CFHashCode)(uintptr_t)cf;
}

CFStringRef CFCopyDescription(CFTypeRef cf)
{
	const CFRuntimeBase *base = cf;
	const CFRuntimeClass *cls = _CFRuntimeGetClass(base->typeID);
	if (cls && cls->copyFormattingDesc)
		return cls->copyFormattingDesc(cf);
	char buf[64];
	snprintf(buf, sizeof(buf), "<%s %p>", cls ? cls->className : "CFType", (const void *)cf);
	return CFStringCreateWithCString(kCFAllocatorDefault, buf, kCFStringEncodingUTF8);
}

CFAllocatorRef CFGetAllocator(CFTypeRef cf)
{
	(void)cf;
	return kCFAllocatorDefault;	/* v1: per-object allocator tracking not implemented, see CFAllocator.c */
}

void CFShow(CFTypeRef obj)
{
	CFStringRef desc = CFCopyDescription(obj);
	char buf[1024];
	CFStringGetCString(desc, buf, sizeof(buf), kCFStringEncodingUTF8);
	fprintf(stderr, "%s\n", buf);
	CFRelease(desc);
}
