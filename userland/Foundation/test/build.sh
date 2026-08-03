#!/bin/bash
# Builds foundationtest: real Objective-C (foundationtest.m), compiled
# with the host's off-the-shelf clang, linked dynamically against
# libFoundation.dylib + libobjc.A.dylib + libCoreFoundation.dylib +
# libSystem.B.dylib -- same discipline as objctest/cftest.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"
ROOT="$(cd ../../.. && pwd)"

CLANG=/Applications/Xcode-beta.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang
OUT="$ROOT/build/Foundation_obj"
mkdir -p "$OUT"

CFLAGS=(-target x86_64-apple-macos10.15 -fobjc-runtime=macosx -fobjc-arc
        -ffreestanding -fno-stack-protector -fno-builtin -nostdlibinc
        -I "$ROOT/userland/Foundation/include" -I "$ROOT/userland/CoreFoundation/include"
        -isystem "$ROOT/userland/libc/include" -O1 -g -Wall -Wextra -Wno-unused-parameter)

"$CLANG" "${CFLAGS[@]}" -c foundationtest.m -o "$OUT/foundationtest_main.o"

# mrc_bridge_helper.m: deliberately NOT -fobjc-arc -- see its own header
# comment (ARC forbids explicit -retain/-release message sends, but
# proving toll-free bridging needs manual control of the retain count).
MRC_CFLAGS=(-target x86_64-apple-macos10.15 -fobjc-runtime=macosx
        -ffreestanding -fno-stack-protector -fno-builtin -nostdlibinc
        -I "$ROOT/userland/Foundation/include" -I "$ROOT/userland/CoreFoundation/include"
        -isystem "$ROOT/userland/libc/include" -O1 -g -Wall -Wextra -Wno-unused-parameter)
"$CLANG" "${MRC_CFLAGS[@]}" -c mrc_bridge_helper.m -o "$OUT/mrc_bridge_helper.o"

CRT0="$ROOT/build/libc_obj/crt0.o"
LIBC_START="$ROOT/build/libc_obj/libc_start.o"
LIBSYSTEM="$ROOT/build/libSystem_obj/libSystem.B.dylib"
LIBOBJC="$ROOT/build/libobjc_obj/libobjc.A.dylib"
LIBCF="$ROOT/build/corefoundation_obj/libCoreFoundation.dylib"
LIBFOUNDATION="$OUT/libFoundation.dylib"

# -no_pie, -bind_at_load: same reasoning as every other test/build.sh in
# this tree (userland/libobjc/test/build.sh, userland/CoreFoundation/test/build.sh).
"$CLANG" -target x86_64-apple-macos10.15 -nostdlib -Wl,-no_pie -Wl,-bind_at_load -e _start \
	"$CRT0" "$LIBC_START" "$OUT/foundationtest_main.o" "$OUT/mrc_bridge_helper.o" "$LIBFOUNDATION" "$LIBOBJC" "$LIBCF" "$LIBSYSTEM" -o "$OUT/foundationtest"

echo "built: $OUT/foundationtest"
file "$OUT/foundationtest"
otool -l "$OUT/foundationtest" | grep -A2 "LC_LOAD_DYLINKER\|LC_LOAD_DYLIB"
