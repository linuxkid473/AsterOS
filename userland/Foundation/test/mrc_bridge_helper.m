/* Copyright (c) 2026 Vihaan Nathan
 *
 * Deliberately NOT -fobjc-arc (see build.sh): ARC forbids explicit
 * -retain/-release message sends at compile time, but proving toll-free
 * bridging means proving CF and NS API move the exact same retain
 * count -- that needs manual control, same reasoning as
 * userland/libobjc/test/mrc_helper.m.
 */
#import <Foundation/Foundation.h>

int
test_bridge_retain_count_identity(void)
{
	CFStringRef cf = CFStringCreateWithCString(kCFAllocatorDefault, "bridge test", kCFStringEncodingUTF8);
	if (CFGetRetainCount(cf) != 1) {
		return 0;
	}
	NSString *ns = (NSString *)cf;
	[ns retain];
	if (CFGetRetainCount(cf) != 2) {
		return 0;
	}
	[ns release];
	if (CFGetRetainCount(cf) != 1) {
		return 0;
	}
	CFRelease(cf);
	return 1;
}
