#!/bin/bash
# Builds libdispatch.dylib: v1-scoped GCD (dispatch_queue_t, dispatch_once,
# dispatch_semaphore_t, dispatch_group_t, dispatch_after) on top of the real
# libSystem.B.dylib (Phase 12) -- same dependency pattern as libobjc/
# CoreFoundation's own builds. Needs -fblocks (block literal codegen) and
# libSystem's now-real Blocks runtime (userland/libSystem/build.sh).
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"
ROOT="$(cd ../.. && pwd)"
LIBSYSTEM="$ROOT/build/libSystem_obj/libSystem.B.dylib"

if [ ! -f "$LIBSYSTEM" ]; then
	echo "error: $LIBSYSTEM not found -- build userland/libSystem first" >&2
	exit 1
fi

CLANG=/Applications/Xcode-beta.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang
OUT="$ROOT/build/dispatch_obj"
mkdir -p "$OUT"

CFLAGS=(-target x86_64-apple-macos10.15 -ffreestanding -fno-stack-protector
        -fno-builtin -fPIC -fblocks -nostdlibinc
        -I "$PWD/include" -isystem "$ROOT/userland/libc/include"
        -O1 -g -Wall -Wextra -Wno-unused-parameter -std=gnu11)

OBJS=()
for f in dispatch_object dispatch_queue dispatch_semaphore dispatch_group dispatch_once dispatch_time dispatch_init; do
	"$CLANG" "${CFLAGS[@]}" -c "$f.c" -o "$OUT/$f.o"
	OBJS+=("$OUT/$f.o")
done

# -bind_at_load: same host ld64 lazy-stub crash as every other dylib in
# this tree (userland/dyld/test/build.sh) -- no real dyld_stub_binder yet.
"$CLANG" -target x86_64-apple-macos10.15 -nostdlib -dynamiclib -Wl,-bind_at_load \
	-install_name /usr/lib/libdispatch.dylib \
	"${OBJS[@]}" "$LIBSYSTEM" -o "$OUT/libdispatch.dylib"

echo "built: $OUT/libdispatch.dylib"
file "$OUT/libdispatch.dylib"
otool -l "$OUT/libdispatch.dylib" | grep -A2 LC_LOAD_DYLIB
