#!/bin/bash
# Builds cftest: a normal dynamically-linked executable exercising
# CFString/CFArray/CFDictionary/CFSet/CFNumber/CFBoolean/CFNull and real
# retain/release refcounting against libCoreFoundation.dylib +
# libSystem.B.dylib -- same pattern as userland/libobjc/test/build.sh.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"
ROOT="$(cd ../../.. && pwd)"

CLANG=/Applications/Xcode-beta.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang
OUT="$ROOT/build/corefoundation_obj"
mkdir -p "$OUT"

CFLAGS=(-target x86_64-apple-macos10.15 -ffreestanding -fno-stack-protector
        -fno-builtin -nostdlibinc -I "$ROOT/userland/CoreFoundation/include"
        -isystem "$ROOT/userland/libc/include" -O1 -g -Wall -Wextra -Wno-unused-parameter)

"$CLANG" "${CFLAGS[@]}" -c cftest.c -o "$OUT/cftest_main.o"

CRT0="$ROOT/build/libc_obj/crt0.o"
LIBC_START="$ROOT/build/libc_obj/libc_start.o"
LIBSYSTEM="$ROOT/build/libSystem_obj/libSystem.B.dylib"
LIBCF="$OUT/libCoreFoundation.dylib"

"$CLANG" -target x86_64-apple-macos10.15 -nostdlib -Wl,-no_pie -Wl,-bind_at_load -e _start \
	"$CRT0" "$LIBC_START" "$OUT/cftest_main.o" "$LIBCF" "$LIBSYSTEM" -o "$OUT/cftest"

echo "built: $OUT/cftest"
file "$OUT/cftest"
otool -l "$OUT/cftest" | grep -A2 LC_LOAD_DYLIB
