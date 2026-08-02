/* End-to-end proof that CoreFoundation's v1 core (object model + string/
 * array/dictionary/set/number/boolean/null) behaves correctly: real
 * retain/release refcounting, real callback-driven collections, not
 * stubs. Same pattern as userland/libSystem/test/test_main.c and
 * userland/pthread_test/pthread_test_main.c -- a normal dynamically-
 * linked executable against the real libSystem.B.dylib +
 * libCoreFoundation.dylib.
 */
#include <CoreFoundation/CoreFoundation.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(cond, msg) \
	do { \
		if (!(cond)) { \
			printf("CFTEST FAIL: %s\n", msg); \
			exit(1); \
		} \
	} while (0)

static void
test_string(void)
{
	CFStringRef s = CFStringCreateWithCString(kCFAllocatorDefault, "hello", kCFStringEncodingUTF8);
	CHECK(CFStringGetLength(s) == 5, "string length");
	CHECK(CFStringGetCharacterAtIndex(s, 0) == 'h', "character at index");

	char buf[32];
	CHECK(CFStringGetCString(s, buf, sizeof(buf), kCFStringEncodingUTF8), "get cstring");
	CHECK(buf[0] == 'h' && buf[4] == 'o' && buf[5] == '\0', "get cstring contents");

	CFStringRef s2 = CFStringCreateWithCString(kCFAllocatorDefault, "hello", kCFStringEncodingUTF8);
	CHECK(CFEqual(s, s2), "string equality");
	CHECK(CFStringCompare(s, s2, 0) == kCFCompareEqualTo, "string compare equal");

	CFMutableStringRef m = CFStringCreateMutable(kCFAllocatorDefault, 0);
	CFStringAppendCString(m, "foo", kCFStringEncodingUTF8);
	CFStringAppend(m, s);
	CHECK(CFStringGetLength(m) == 8, "mutable append length");
	CFStringGetCString(m, buf, sizeof(buf), kCFStringEncodingUTF8);
	CHECK(buf[0] == 'f' && buf[3] == 'h', "mutable append contents");

	CHECK(CFStringHasPrefix(m, s2) == false, "prefix negative");
	CFStringRef pfx = CFStringCreateWithCString(kCFAllocatorDefault, "foo", kCFStringEncodingUTF8);
	CHECK(CFStringHasPrefix(m, pfx), "prefix positive");

	/* no %f/%e/%g here: this OS's own libc vsnprintf has no floating-point
	 * conversion support at all (see CFString.c's CreateWithFormat header
	 * comment) -- a pre-existing libc limitation this delegates to, not
	 * something to work around in CoreFoundation itself. */
	CFStringRef fmtSpec = CFStringCreateWithCString(kCFAllocatorDefault, "n=%d s=%s x=%x", kCFStringEncodingUTF8);
	CFStringRef fmt = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, fmtSpec, 42, "hi", 255);
	CFStringGetCString(fmt, buf, sizeof(buf), kCFStringEncodingUTF8);
	CHECK(strcmp(buf, "n=42 s=hi x=ff") == 0, "CFStringCreateWithFormat");

	CFRelease(s);
	CFRelease(s2);
	CFRelease(m);
	CFRelease(pfx);
	CFRelease(fmtSpec);
	CFRelease(fmt);
	printf("CFTEST: CFString ok\n");
}

static void
test_array(void)
{
	CFMutableArrayRef arr = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
	CFStringRef a = CFStringCreateWithCString(kCFAllocatorDefault, "a", kCFStringEncodingUTF8);
	CFStringRef b = CFStringCreateWithCString(kCFAllocatorDefault, "b", kCFStringEncodingUTF8);
	CFArrayAppendValue(arr, a);
	CFArrayAppendValue(arr, b);
	CHECK(CFGetRetainCount(a) == 2, "array retained appended value");
	CHECK(CFArrayGetCount(arr) == 2, "array count");
	CHECK(CFArrayGetValueAtIndex(arr, 0) == a, "array value at index");
	CHECK(CFArrayContainsValue(arr, CFRangeMake(0, 2), b), "array contains");

	CFArrayRemoveValueAtIndex(arr, 0);
	CHECK(CFArrayGetCount(arr) == 1, "array count after remove");
	CHECK(CFGetRetainCount(a) == 1, "array released removed value");

	CFRelease(a);
	CFRelease(b);
	CFRelease(arr);
	printf("CFTEST: CFArray ok\n");
}

static void
test_dictionary_set(void)
{
	CFMutableDictionaryRef dict = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	CFStringRef key = CFStringCreateWithCString(kCFAllocatorDefault, "key", kCFStringEncodingUTF8);
	CFStringRef val = CFStringCreateWithCString(kCFAllocatorDefault, "value", kCFStringEncodingUTF8);
	CFDictionarySetValue(dict, key, val);
	CHECK(CFDictionaryGetCount(dict) == 1, "dict count");
	CHECK(CFDictionaryContainsKey(dict, key), "dict contains key");

	CFStringRef key2 = CFStringCreateWithCString(kCFAllocatorDefault, "key", kCFStringEncodingUTF8);
	const void *found = CFDictionaryGetValue(dict, key2);
	CHECK(found != NULL && CFEqual((CFTypeRef)found, val), "dict lookup by equal-but-not-identical key");

	CFDictionaryRemoveValue(dict, key);
	CHECK(CFDictionaryGetCount(dict) == 0, "dict count after remove");

	CFMutableSetRef set = CFSetCreateMutable(kCFAllocatorDefault, 0, &kCFTypeSetCallBacks);
	CFSetAddValue(set, key2);
	CFSetAddValue(set, key2);
	CHECK(CFSetGetCount(set) == 1, "set dedups equal values");
	CHECK(CFSetContainsValue(set, val) == false, "set doesn't contain unrelated value");

	CFRelease(key);
	CFRelease(val);
	CFRelease(key2);
	CFRelease(dict);
	CFRelease(set);
	printf("CFTEST: CFDictionary/CFSet ok\n");
}

static void
test_number_boolean_null(void)
{
	int iv = 42;
	CFNumberRef n = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &iv);
	int back = 0;
	CHECK(CFNumberGetValue(n, kCFNumberIntType, &back) && back == 42, "number int roundtrip");

	double dv = 2.5;
	CFNumberRef nd = CFNumberCreate(kCFAllocatorDefault, kCFNumberDoubleType, &dv);
	CHECK(CFNumberCompare(n, nd, NULL) == kCFCompareGreaterThan, "number compare int vs double");

	CHECK(CFBooleanGetValue(kCFBooleanTrue), "boolean true");
	CHECK(!CFBooleanGetValue(kCFBooleanFalse), "boolean false");
	CHECK(CFGetTypeID(kCFNull) == CFNullGetTypeID(), "null type id");

	CFRelease(n);
	CFRelease(nd);
	printf("CFTEST: CFNumber/CFBoolean/CFNull ok\n");
}

static void
test_retain_release(void)
{
	CFStringRef s = CFStringCreateWithCString(kCFAllocatorDefault, "rc", kCFStringEncodingUTF8);
	CHECK(CFGetRetainCount(s) == 1, "initial retain count");
	CFRetain(s);
	CHECK(CFGetRetainCount(s) == 2, "retain increments");
	CFRelease(s);
	CHECK(CFGetRetainCount(s) == 1, "release decrements");
	CFRelease(s);	/* drops to 0, frees -- nothing observable but must not crash */
	printf("CFTEST: retain/release ok\n");
}

int
main(void)
{
	printf("CFTEST: starting\n");
	test_string();
	test_array();
	test_dictionary_set();
	test_number_boolean_null();
	test_retain_release();
	printf("CFTEST PASS\n");
	return 0;
}
