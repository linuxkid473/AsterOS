/* Copyright (c) 2026 Vihaan Nathan -- see CFTimeZone.h */
#include "CFInternal.h"
#include <string.h>
#include <pthread.h>

struct __CFTimeZone {
	CFRuntimeBase base;
	CFStringRef name;
};

static CFTypeID g_tzTypeID;
static pthread_once_t g_tzOnce = PTHREAD_ONCE_INIT;

static void tzFinalize(CFTypeRef cf)
{
	CFRelease(((const struct __CFTimeZone *)cf)->name);
}

static CFStringRef tzCopyDesc(CFTypeRef cf)
{
	return CFRetain(((const struct __CFTimeZone *)cf)->name);
}

static void tzInit(void)
{
	static const CFRuntimeClass cls = {
		.className = "CFTimeZone",
		.finalize = tzFinalize,
		.copyFormattingDesc = tzCopyDesc,
	};
	g_tzTypeID = _CFRuntimeRegisterClass(&cls);
}

CFTypeID CFTimeZoneGetTypeID(void)
{
	pthread_once(&g_tzOnce, tzInit);
	return g_tzTypeID;
}

CFTimeZoneRef CFTimeZoneCreateWithName(CFAllocatorRef allocator, CFStringRef name, Boolean tryAbbrev)
{
	(void)tryAbbrev;
	char buf[64];
	if (!CFStringGetCString(name, buf, sizeof(buf), kCFStringEncodingUTF8))
		return NULL;
	if (strcmp(buf, "UTC") != 0 && strcmp(buf, "GMT") != 0)
		return NULL;	/* no tzdata to resolve anything else -- see header comment */
	CFTimeZoneGetTypeID();
	struct __CFTimeZone *tz = (struct __CFTimeZone *)_CFRuntimeCreateInstance(allocator, g_tzTypeID, sizeof(struct __CFTimeZone) - sizeof(CFRuntimeBase));
	tz->name = CFStringCreateCopy(kCFAllocatorDefault, name);
	return (CFTimeZoneRef)tz;
}

CFTimeZoneRef CFTimeZoneCopySystem(void)
{
	CFStringRef utc = CFStringCreateWithCString(kCFAllocatorDefault, "UTC", kCFStringEncodingUTF8);
	CFTimeZoneRef tz = CFTimeZoneCreateWithName(kCFAllocatorDefault, utc, false);
	CFRelease(utc);
	return tz;
}

CFTimeZoneRef CFTimeZoneCopyDefault(void)
{
	return CFTimeZoneCopySystem();
}

CFStringRef CFTimeZoneGetName(CFTimeZoneRef tz)
{
	return ((const struct __CFTimeZone *)tz)->name;
}

CFTimeInterval CFTimeZoneGetSecondsFromGMT(CFTimeZoneRef tz, CFAbsoluteTime at)
{
	(void)tz;
	(void)at;
	return 0.0;	/* always UTC, see header comment */
}
