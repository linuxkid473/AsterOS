#!/bin/bash
# Builds dispatchtest: exercises dispatch_queue_t/dispatch_once/
# dispatch_semaphore_t/dispatch_group_t/dispatch_after against real
# libdispatch.dylib + libSystem.B.dylib -- same pattern as
# userland/CoreFoundation/test/build.sh.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"
ROOT="$(cd ../../.. && pwd)"

CLANG=/Applications/Xcode-beta.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang
OUT="$ROOT/build/dispatch_obj"
mkdir -p "$OUT"

CFLAGS=(-target x86_64-apple-macos10.15 -ffreestanding -fno-stack-protector
        -fno-builtin -fblocks -nostdlibinc -I "$ROOT/userland/libdispatch/include"
        -isystem "$ROOT/userland/libc/include" -O1 -g -Wall -Wextra -Wno-unused-parameter)

"$CLANG" "${CFLAGS[@]}" -c dispatchtest.c -o "$OUT/dispatchtest_main.o"

CRT0="$ROOT/build/libc_obj/crt0.o"
LIBC_START="$ROOT/build/libc_obj/libc_start.o"
LIBSYSTEM="$ROOT/build/libSystem_obj/libSystem.B.dylib"
LIBDISPATCH="$OUT/libdispatch.dylib"

"$CLANG" -target x86_64-apple-macos10.15 -nostdlib -Wl,-no_pie -Wl,-bind_at_load -e _start \
	"$CRT0" "$LIBC_START" "$OUT/dispatchtest_main.o" "$LIBDISPATCH" "$LIBSYSTEM" -o "$OUT/dispatchtest"

echo "built: $OUT/dispatchtest"
file "$OUT/dispatchtest"
otool -l "$OUT/dispatchtest" | grep -A2 LC_LOAD_DYLIB
