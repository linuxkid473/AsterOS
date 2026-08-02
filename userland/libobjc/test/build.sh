#!/bin/bash
# Builds objctest: real Objective-C (test.m), compiled with the host's
# off-the-shelf clang exactly as any real .m file would be -- no source
# modifications, no hand-authored metadata -- and linked dynamically
# against libobjc.A.dylib + libSystem.B.dylib. This is the actual
# Phase 13 regression test: if this doesn't behave correctly, the
# runtime's ABI compatibility claim doesn't hold.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"
ROOT="$(cd ../../.. && pwd)"

CLANG=/Applications/Xcode-beta.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang
OUT="$ROOT/build/libobjc_obj"
mkdir -p "$OUT"

CFLAGS=(-target x86_64-apple-macos10.15 -fobjc-runtime=macosx -fobjc-arc
        -ffreestanding -fno-stack-protector -fno-builtin -nostdlibinc
        -isystem "$ROOT/userland/libc/include" -O1 -g -Wall -Wextra -Wno-unused-parameter)

"$CLANG" "${CFLAGS[@]}" -c test.m -o "$OUT/test_main.o"

# mrc_helper.m: deliberately NOT -fobjc-arc -- see its own header comment
# and test.m's comment at test_mrc_autorelease's declaration.
MRC_CFLAGS=(-target x86_64-apple-macos10.15 -fobjc-runtime=macosx
        -ffreestanding -fno-stack-protector -fno-builtin -nostdlibinc
        -isystem "$ROOT/userland/libc/include" -O1 -g -Wall -Wextra -Wno-unused-parameter)
"$CLANG" "${MRC_CFLAGS[@]}" -c mrc_helper.m -o "$OUT/mrc_helper.o"

CRT0="$ROOT/build/libc_obj/crt0.o"
LIBC_START="$ROOT/build/libc_obj/libc_start.o"
LIBSYSTEM="$ROOT/build/libSystem_obj/libSystem.B.dylib"
LIBOBJC="$OUT/libobjc.A.dylib"

# -no_pie, -bind_at_load: same reasoning as userland/dyld/test/build.sh
# and userland/libSystem/test/build.sh.
"$CLANG" -target x86_64-apple-macos10.15 -nostdlib -Wl,-no_pie -Wl,-bind_at_load -e _start \
	"$CRT0" "$LIBC_START" "$OUT/test_main.o" "$OUT/mrc_helper.o" "$LIBOBJC" "$LIBSYSTEM" -o "$OUT/objctest"

echo "built: $OUT/objctest"
file "$OUT/objctest"
otool -l "$OUT/objctest" | grep -A2 "LC_LOAD_DYLINKER\|LC_LOAD_DYLIB"
