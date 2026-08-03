#!/bin/bash
# Builds libCoreFoundation.dylib: depends on the real libSystem.B.dylib
# (Phase 12) exactly like libobjc's build does, but has no dependency on
# libobjc.A.dylib itself -- this is a plain-C object model (see
# CFInternal.h's header comment for why), not built on top of the
# Objective-C runtime the way real CF optionally can be.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"
ROOT="$(cd ../.. && pwd)"
LIBSYSTEM="$ROOT/build/libSystem_obj/libSystem.B.dylib"

if [ ! -f "$LIBSYSTEM" ]; then
	echo "error: $LIBSYSTEM not found -- build userland/libSystem first" >&2
	exit 1
fi

CLANG=/Applications/Xcode-beta.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang
OUT="$ROOT/build/corefoundation_obj"
mkdir -p "$OUT"

CFLAGS=(-target x86_64-apple-macos10.15 -ffreestanding -fno-stack-protector
        -fno-builtin -fPIC -nostdlibinc
        -I "$PWD/include" -isystem "$ROOT/userland/libc/include"
        -O1 -g -Wall -Wextra -Wno-unused-parameter -std=gnu11)

OBJS=()
for f in CFRuntime CFAllocator CFNull CFBoolean CFString CFArray CFDictionary CFSet CFNumber CFData CFDate CFTimeZone CFLocale CFURL; do
	"$CLANG" "${CFLAGS[@]}" -c "$f.c" -o "$OUT/$f.o"
	OBJS+=("$OUT/$f.o")
done

# -bind_at_load: same host ld64 lazy-stub crash as every other dylib in
# this tree (userland/dyld/test/build.sh) -- no real dyld_stub_binder yet.
"$CLANG" -target x86_64-apple-macos10.15 -nostdlib -dynamiclib -Wl,-bind_at_load \
	-install_name /usr/lib/libCoreFoundation.dylib \
	"${OBJS[@]}" "$LIBSYSTEM" -o "$OUT/libCoreFoundation.dylib"

echo "built: $OUT/libCoreFoundation.dylib"
file "$OUT/libCoreFoundation.dylib"
otool -l "$OUT/libCoreFoundation.dylib" | grep -A2 LC_LOAD_DYLIB
