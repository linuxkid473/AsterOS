/* Copyright (c) 2026 Vihaan Nathan */
#include "CFInternal.h"

struct __CFNull {
	CFRuntimeBase base;
};

static CFTypeID g_nullTypeID;
static struct __CFNull g_null;

static CFStringRef nullCopyDesc(CFTypeRef cf)
{
	(void)cf;
	return CFStringCreateWithCString(kCFAllocatorDefault, "<CFNull null>", kCFStringEncodingUTF8);
}

/* See CFAllocator.c's header comment: kCFNull must be valid before any
 * other CF call, so it self-registers at image-load time. */
__attribute__((constructor))
static void nullInit(void)
{
	static const CFRuntimeClass cls = {
		.className = "CFNull",
		.copyFormattingDesc = nullCopyDesc,
	};
	g_nullTypeID = _CFRuntimeRegisterClass(&cls);
	_CFRuntimeInitStaticInstance(&g_null, g_nullTypeID);
}

const CFNullRef kCFNull = (CFNullRef)&g_null;

CFTypeID CFNullGetTypeID(void)
{
	return g_nullTypeID;
}
