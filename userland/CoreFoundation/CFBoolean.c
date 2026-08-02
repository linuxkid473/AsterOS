/* Copyright (c) 2026 Vihaan Nathan */
#include "CFInternal.h"

struct __CFBoolean {
	CFRuntimeBase base;
	Boolean value;
};

static CFTypeID g_booleanTypeID;
static struct __CFBoolean g_true = { .value = true };
static struct __CFBoolean g_false = { .value = false };

static Boolean booleanEqual(CFTypeRef cf1, CFTypeRef cf2)
{
	return ((const struct __CFBoolean *)cf1)->value == ((const struct __CFBoolean *)cf2)->value;
}

static CFHashCode booleanHash(CFTypeRef cf)
{
	return ((const struct __CFBoolean *)cf)->value ? 1 : 0;
}

static CFStringRef booleanCopyDesc(CFTypeRef cf)
{
	Boolean v = ((const struct __CFBoolean *)cf)->value;
	return CFStringCreateWithCString(kCFAllocatorDefault, v ? "true" : "false", kCFStringEncodingUTF8);
}

/* See CFAllocator.c's header comment: kCFBooleanTrue/False must be
 * valid before any other CF call, so they self-register at image-load
 * time rather than lazily on first CFBooleanGetTypeID() call. */
__attribute__((constructor))
static void booleanInit(void)
{
	static const CFRuntimeClass cls = {
		.className = "CFBoolean",
		.equal = booleanEqual,
		.hash = booleanHash,
		.copyFormattingDesc = booleanCopyDesc,
	};
	g_booleanTypeID = _CFRuntimeRegisterClass(&cls);
	_CFRuntimeInitStaticInstance(&g_true, g_booleanTypeID);
	_CFRuntimeInitStaticInstance(&g_false, g_booleanTypeID);
	g_true.value = true;
	g_false.value = false;
}

const CFBooleanRef kCFBooleanTrue = (CFBooleanRef)&g_true;
const CFBooleanRef kCFBooleanFalse = (CFBooleanRef)&g_false;

CFTypeID CFBooleanGetTypeID(void)
{
	return g_booleanTypeID;
}

Boolean CFBooleanGetValue(CFBooleanRef boolean)
{
	return boolean->value;
}
