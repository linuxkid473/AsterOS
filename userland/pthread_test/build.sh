#!/bin/bash
# Builds pthreadtest: a normal dynamically-linked executable proving real
# pthread_create()/mutex/cond work end to end against the real
# libSystem.B.dylib -- same pattern as userland/libSystem/test/build.sh.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"
ROOT="$(cd ../.. && pwd)"

CLANG=/Applications/Xcode-beta.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang
OUT="$ROOT/build/libSystem_obj"
mkdir -p "$OUT"

CFLAGS=(-target x86_64-apple-macos10.15 -ffreestanding -fno-stack-protector
        -fno-builtin -nostdlibinc -isystem "$ROOT/userland/libc/include"
        -O1 -g -Wall -Wextra -Wno-unused-parameter)

"$CLANG" "${CFLAGS[@]}" -c pthread_test_main.c -o "$OUT/pthreadtest_main.o"

CRT0="$ROOT/build/libc_obj/crt0.o"
LIBC_START="$ROOT/build/libc_obj/libc_start.o"

"$CLANG" -target x86_64-apple-macos10.15 -nostdlib -Wl,-no_pie -Wl,-bind_at_load -e _start \
	"$CRT0" "$LIBC_START" "$OUT/pthreadtest_main.o" "$OUT/libSystem.B.dylib" -o "$OUT/pthreadtest"

echo "built: $OUT/pthreadtest"
file "$OUT/pthreadtest"
