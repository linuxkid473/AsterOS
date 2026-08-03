#!/bin/bash
# Builds libFoundation.dylib: depends on libobjc.A.dylib (Phase 13),
# libCoreFoundation.dylib (Phase 17), and libSystem.B.dylib (Phase 12) --
# real Objective-C classes toll-free bridged to CF, see docs/architecture.md.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"
ROOT="$(cd ../.. && pwd)"
LIBSYSTEM="$ROOT/build/libSystem_obj/libSystem.B.dylib"
LIBOBJC="$ROOT/build/libobjc_obj/libobjc.A.dylib"
LIBCOREFOUNDATION="$ROOT/build/corefoundation_obj/libCoreFoundation.dylib"

for dep in "$LIBSYSTEM" "$LIBOBJC" "$LIBCOREFOUNDATION"; do
	if [ ! -f "$dep" ]; then
		echo "error: $dep not found -- build its phase first" >&2
		exit 1
	fi
done

CLANG=/Applications/Xcode-beta.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang
OUT="$ROOT/build/Foundation_obj"
mkdir -p "$OUT"

CFLAGS=(-target x86_64-apple-macos10.15 -fobjc-runtime=macosx -fno-objc-arc
        -ffreestanding -fno-stack-protector -fno-builtin -fPIC -nostdlibinc
        -I "$PWD/include" -I "$ROOT/userland/CoreFoundation/include"
        -isystem "$ROOT/userland/libc/include"
        -O1 -g -Wall -Wextra -Wno-unused-parameter -Wno-deprecated-declarations
        -Wno-incomplete-implementation -Wno-objc-method-access
        -Wno-objc-missing-super-calls
        # -Wno-cast-function-type-mismatch: the standard objc_msgSend
        # recast-to-a-fixed-signature idiom (NSNotification.m/NSTimer.m)
        # always trips this -- objc_msgSend's declared type is variadic,
        # every real caller recasts it to the actual argument shape being
        # sent. Not a real type-safety issue: the ABI is identical either
        # way (System V x86_64 doesn't distinguish variadic vs. fixed
        # integer/pointer argument passing).
        -Wno-cast-function-type-mismatch
        # -Wno-protocol: NSCoding's encodeWithCoder:/initWithCoder: are
        # implemented once NSCoder/NSKeyedArchiver land later in this same
        # phase (see TODO.md) -- every class declares <NSCoding> conformance
        # now to match real Foundation's public API, satisfied for real once
        # that milestone lands, so this suppression is temporary within the
        # phase, not a permanent gap.
        -Wno-protocol)

OBJS=()
for f in FoundationInit NSCFBridge NSObject NSString NSNumber NSNull NSArray NSDictionary NSSet NSData NSException NSError NSDate NSTimeZone NSLocale NSURL NSProcessInfo NSFileManager NSBundle NSNotification NSTimer NSRunLoop NSPropertyListSerialization NSJSONSerialization NSCoder NSKeyedArchiver NSUserDefaults; do
	"$CLANG" "${CFLAGS[@]}" -c "$f.m" -o "$OUT/$f.o"
	OBJS+=("$OUT/$f.o")
done

# -bind_at_load: same host ld64 lazy-stub crash as every other dylib in
# this tree (userland/dyld/test/build.sh) -- no real dyld_stub_binder yet.
"$CLANG" -target x86_64-apple-macos10.15 -nostdlib -dynamiclib -Wl,-bind_at_load \
	-install_name /usr/lib/libFoundation.dylib \
	"${OBJS[@]}" "$LIBOBJC" "$LIBCOREFOUNDATION" "$LIBSYSTEM" -o "$OUT/libFoundation.dylib"

echo "built: $OUT/libFoundation.dylib"
file "$OUT/libFoundation.dylib"
otool -l "$OUT/libFoundation.dylib" | grep -A2 LC_LOAD_DYLIB
