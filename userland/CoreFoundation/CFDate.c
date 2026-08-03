/* Copyright (c) 2026 Vihaan Nathan -- see CFDate.h */
#include "CFInternal.h"
#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>

struct __CFDate {
	CFRuntimeBase base;
	CFAbsoluteTime at;
};

static CFTypeID g_dateTypeID;
static pthread_once_t g_dateOnce = PTHREAD_ONCE_INIT;

CFAbsoluteTime
CFAbsoluteTimeGetCurrent(void)
{
	struct timeval tv;
	gettimeofday(&tv, 0);
	return ((double)tv.tv_sec - kCFAbsoluteTimeIntervalSince1970) + (double)tv.tv_usec / 1000000.0;
}

static Boolean dateEqual(CFTypeRef cf1, CFTypeRef cf2)
{
	return ((const struct __CFDate *)cf1)->at == ((const struct __CFDate *)cf2)->at;
}

static CFHashCode dateHash(CFTypeRef cf)
{
	return (CFHashCode)((const struct __CFDate *)cf)->at;
}

static CFStringRef dateCopyDesc(CFTypeRef cf)
{
	char buf[64];
	snprintf(buf, sizeof(buf), "<CFDate %g>", ((const struct __CFDate *)cf)->at);
	return CFStringCreateWithCString(kCFAllocatorDefault, buf, kCFStringEncodingUTF8);
}

static void dateInit(void)
{
	static const CFRuntimeClass cls = {
		.className = "CFDate",
		.equal = dateEqual,
		.hash = dateHash,
		.copyFormattingDesc = dateCopyDesc,
	};
	g_dateTypeID = _CFRuntimeRegisterClass(&cls);
}

CFTypeID CFDateGetTypeID(void)
{
	pthread_once(&g_dateOnce, dateInit);
	return g_dateTypeID;
}

CFDateRef CFDateCreate(CFAllocatorRef allocator, CFAbsoluteTime at)
{
	CFDateGetTypeID();
	struct __CFDate *d = (struct __CFDate *)_CFRuntimeCreateInstance(allocator, g_dateTypeID, sizeof(struct __CFDate) - sizeof(CFRuntimeBase));
	d->at = at;
	return (CFDateRef)d;
}

CFAbsoluteTime CFDateGetAbsoluteTime(CFDateRef theDate)
{
	return ((const struct __CFDate *)theDate)->at;
}

CFTimeInterval CFDateGetTimeIntervalSinceDate(CFDateRef theDate, CFDateRef otherDate)
{
	return ((const struct __CFDate *)theDate)->at - ((const struct __CFDate *)otherDate)->at;
}

CFComparisonResult CFDateCompare(CFDateRef theDate, CFDateRef otherDate, void *context)
{
	(void)context;
	CFAbsoluteTime a = ((const struct __CFDate *)theDate)->at;
	CFAbsoluteTime b = ((const struct __CFDate *)otherDate)->at;
	if (a < b)
		return kCFCompareLessThan;
	if (a > b)
		return kCFCompareGreaterThan;
	return kCFCompareEqualTo;
}
