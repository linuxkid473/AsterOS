/* Minimal SDK smoke test: proves the Foundation development headers
 * installed under /usr/include resolve via a plain #import, and that
 * a program built entirely by the on-target clang/ld (not host-cross-
 * compiled) can dynamically link libFoundation.dylib + libCoreFoundation
 * .dylib + libobjc.A.dylib + libSystem.B.dylib and run. See
 * userland/toolchain/build-hello.sh for the invocation.
 *
 * No `@"literal"` NSString constants (see foundationtest.m's own header
 * comment / docs/architecture.md) -- [NSString stringWithUTF8String:]
 * instead.
 */
#import <Foundation/Foundation.h>
#include <stdio.h>

int
main(void)
{
	NSString *greeting = [NSString stringWithUTF8String:"Hello, Foundation!"];
	NSString *name = [[NSProcessInfo processInfo] processName];
	printf("%s\n", [greeting UTF8String]);
	printf("(process name: %s)\n", [name UTF8String]);
	return 0;
}
