/* Copyright (c) 2026 Vihaan Nathan -- see CFURL.h */
#include "CFInternal.h"
#include <string.h>
#include <stdio.h>
#include <pthread.h>

struct __CFURL {
	CFRuntimeBase base;
	CFStringRef path;	/* plain POSIX path, no "file://" prefix -- see header comment */
	Boolean isDirectory;
};

static CFTypeID g_urlTypeID;
static pthread_once_t g_urlOnce = PTHREAD_ONCE_INIT;

static void urlFinalize(CFTypeRef cf)
{
	CFRelease(((const struct __CFURL *)cf)->path);
}

static Boolean urlEqual(CFTypeRef cf1, CFTypeRef cf2)
{
	const struct __CFURL *a = cf1, *b = cf2;
	return a->isDirectory == b->isDirectory && CFEqual(a->path, b->path);
}

static CFHashCode urlHash(CFTypeRef cf)
{
	return CFHash(((const struct __CFURL *)cf)->path);
}

static CFStringRef urlCopyDesc(CFTypeRef cf)
{
	return CFURLGetString((CFURLRef)cf);
}

static void urlInit(void)
{
	static const CFRuntimeClass cls = {
		.className = "CFURL",
		.finalize = urlFinalize,
		.equal = urlEqual,
		.hash = urlHash,
		.copyFormattingDesc = urlCopyDesc,
	};
	g_urlTypeID = _CFRuntimeRegisterClass(&cls);
}

CFTypeID CFURLGetTypeID(void)
{
	pthread_once(&g_urlOnce, urlInit);
	return g_urlTypeID;
}

CFURLRef CFURLCreateWithFileSystemPath(CFAllocatorRef allocator, CFStringRef filePath, CFURLPathStyle pathStyle, Boolean isDirectory)
{
	(void)pathStyle;	/* only kCFURLPOSIXPathStyle exists -- see header */
	CFURLGetTypeID();
	struct __CFURL *u = (struct __CFURL *)_CFRuntimeCreateInstance(allocator, g_urlTypeID, sizeof(struct __CFURL) - sizeof(CFRuntimeBase));
	u->path = CFStringCreateCopy(kCFAllocatorDefault, filePath);
	u->isDirectory = isDirectory;
	return (CFURLRef)u;
}

CFURLRef CFURLCreateWithString(CFAllocatorRef allocator, CFStringRef URLString, CFURLRef baseURL)
{
	(void)baseURL;	/* no relative-URL resolution -- see header */
	char buf[1024];
	if (!CFStringGetCString(URLString, buf, sizeof(buf), kCFStringEncodingUTF8))
		return NULL;
	static const char prefix[] = "file://";
	if (strncmp(buf, prefix, sizeof(prefix) - 1) != 0)
		return NULL;	/* only the file:// scheme is supported */
	const char *path = buf + sizeof(prefix) - 1;
	size_t len = strlen(path);
	Boolean isDir = len > 0 && path[len - 1] == '/';
	CFStringRef pathStr = CFStringCreateWithCString(kCFAllocatorDefault, path, kCFStringEncodingUTF8);
	CFURLRef url = CFURLCreateWithFileSystemPath(allocator, pathStr, kCFURLPOSIXPathStyle, isDir);
	CFRelease(pathStr);
	return url;
}

CFStringRef CFURLGetString(CFURLRef url)
{
	const struct __CFURL *u = (const struct __CFURL *)url;
	char pathBuf[1024];
	CFStringGetCString(u->path, pathBuf, sizeof(pathBuf), kCFStringEncodingUTF8);
	char buf[1088];
	snprintf(buf, sizeof(buf), "file://%s", pathBuf);
	return CFStringCreateWithCString(kCFAllocatorDefault, buf, kCFStringEncodingUTF8);
}

CFStringRef CFURLCopyFileSystemPath(CFURLRef url, CFURLPathStyle pathStyle)
{
	(void)pathStyle;
	return CFStringCreateCopy(kCFAllocatorDefault, ((const struct __CFURL *)url)->path);
}

CFStringRef CFURLCopyLastPathComponent(CFURLRef url)
{
	const struct __CFURL *u = (const struct __CFURL *)url;
	char buf[1024];
	CFStringGetCString(u->path, buf, sizeof(buf), kCFStringEncodingUTF8);
	size_t len = strlen(buf);
	while (len > 1 && buf[len - 1] == '/')
		buf[--len] = '\0';
	const char *slash = strrchr(buf, '/');
	return CFStringCreateWithCString(kCFAllocatorDefault, slash ? slash + 1 : buf, kCFStringEncodingUTF8);
}

CFStringRef CFURLCopyPathExtension(CFURLRef url)
{
	CFStringRef last = CFURLCopyLastPathComponent(url);
	char buf[512];
	CFStringGetCString(last, buf, sizeof(buf), kCFStringEncodingUTF8);
	CFRelease(last);
	const char *dot = strrchr(buf, '.');
	if (!dot || dot == buf)
		return CFStringCreateWithCString(kCFAllocatorDefault, "", kCFStringEncodingUTF8);
	return CFStringCreateWithCString(kCFAllocatorDefault, dot + 1, kCFStringEncodingUTF8);
}

Boolean CFURLHasDirectoryPath(CFURLRef url)
{
	return ((const struct __CFURL *)url)->isDirectory;
}
