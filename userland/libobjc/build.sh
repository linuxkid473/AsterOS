#!/bin/bash
# Builds libobjc.A.dylib: depends on the real libSystem.B.dylib (Phase 12),
# so unlike libSystem's own build this needs no self-link workaround --
# it's a completely ordinary dependency edge, ld64's "must link something
# named libSystem" requirement is satisfied for free.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"
ROOT="$(cd ../.. && pwd)"
LIBSYSTEM="$ROOT/build/libSystem_obj/libSystem.B.dylib"

if [ ! -f "$LIBSYSTEM" ]; then
	echo "error: $LIBSYSTEM not found -- build userland/libSystem first" >&2
	exit 1
fi

CLANG=/Applications/Xcode-beta.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang
OUT="$ROOT/build/libobjc_obj"
mkdir -p "$OUT"

CFLAGS=(-target x86_64-apple-macos10.15 -ffreestanding -fno-stack-protector
        -fno-builtin -fPIC -nostdlibinc -isystem "$ROOT/userland/libc/include"
        -O1 -g -Wall -Wextra -Wno-unused-parameter -Wno-deprecated-declarations)

OBJS=()
for f in selector class dispatch runtime arc autorelease init; do
	"$CLANG" "${CFLAGS[@]}" -c "$f.c" -o "$OUT/$f.o"
	OBJS+=("$OUT/$f.o")
done

"$CLANG" -target x86_64-apple-macos10.15 -fPIC -g -c msgSend.S -o "$OUT/msgSend.o"
OBJS+=("$OUT/msgSend.o")

# Root.m / NSAutoreleasePool.m: real Objective-C, compiled without ARC
# (both implement retain/release/dealloc-adjacent primitives directly)
# but with the same -fobjc-runtime=macosx target as any client -- see
# Root.m's own header comment.
"$CLANG" "${CFLAGS[@]}" -fobjc-runtime=macosx -fno-objc-arc -c Root.m -o "$OUT/Root.o"
OBJS+=("$OUT/Root.o")
"$CLANG" "${CFLAGS[@]}" -fobjc-runtime=macosx -fno-objc-arc -c NSAutoreleasePool.m -o "$OUT/NSAutoreleasePool.o"
OBJS+=("$OUT/NSAutoreleasePool.o")

# -bind_at_load: same host ld64 lazy-stub crash as userland/dyld/test/
# build.sh (IndirectSymbolTableBuilderImpl _sideInfo assertion) --
# nothing exports a real dyld_stub_binder yet, real libSystem or not.
"$CLANG" -target x86_64-apple-macos10.15 -nostdlib -dynamiclib -Wl,-bind_at_load \
	-install_name /usr/lib/libobjc.A.dylib \
	"${OBJS[@]}" "$LIBSYSTEM" -o "$OUT/libobjc.A.dylib"

echo "built: $OUT/libobjc.A.dylib"
file "$OUT/libobjc.A.dylib"
otool -l "$OUT/libobjc.A.dylib" | grep -A2 LC_LOAD_DYLIB
