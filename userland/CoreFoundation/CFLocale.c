/* Copyright (c) 2026 Vihaan Nathan -- see CFLocale.h */
#include "CFInternal.h"
#include <pthread.h>

struct __CFLocale {
	CFRuntimeBase base;
	CFStringRef identifier;
};

static CFTypeID g_localeTypeID;
static pthread_once_t g_localeOnce = PTHREAD_ONCE_INIT;

static void localeFinalize(CFTypeRef cf)
{
	CFRelease(((const struct __CFLocale *)cf)->identifier);
}

static CFStringRef localeCopyDesc(CFTypeRef cf)
{
	return CFRetain(((const struct __CFLocale *)cf)->identifier);
}

static void localeInit(void)
{
	static const CFRuntimeClass cls = {
		.className = "CFLocale",
		.finalize = localeFinalize,
		.copyFormattingDesc = localeCopyDesc,
	};
	g_localeTypeID = _CFRuntimeRegisterClass(&cls);
}

CFTypeID CFLocaleGetTypeID(void)
{
	pthread_once(&g_localeOnce, localeInit);
	return g_localeTypeID;
}

CFLocaleRef CFLocaleCreate(CFAllocatorRef allocator, CFStringRef localeIdentifier)
{
	CFLocaleGetTypeID();
	struct __CFLocale *l = (struct __CFLocale *)_CFRuntimeCreateInstance(allocator, g_localeTypeID, sizeof(struct __CFLocale) - sizeof(CFRuntimeBase));
	l->identifier = CFStringCreateCopy(kCFAllocatorDefault, localeIdentifier);
	return (CFLocaleRef)l;
}

CFLocaleRef CFLocaleCopyCurrent(void)
{
	CFStringRef ident = CFStringCreateWithCString(kCFAllocatorDefault, "en_US_POSIX", kCFStringEncodingUTF8);
	CFLocaleRef l = CFLocaleCreate(kCFAllocatorDefault, ident);
	CFRelease(ident);
	return l;
}

CFLocaleRef CFLocaleGetSystem(void)
{
	/* Lazily created, not pthread_once-guarded like the constant
	 * singletons (kCFNull etc.) -- this allocates through the ordinary
	 * CF path rather than a static init function, and this tree has no
	 * concurrent callers of this specific accessor to race. A documented
	 * simplification, not an oversight. */
	static CFLocaleRef sys;
	if (!sys)
		sys = CFLocaleCopyCurrent();
	return sys;
}

CFStringRef CFLocaleGetIdentifier(CFLocaleRef locale)
{
	return ((const struct __CFLocale *)locale)->identifier;
}
