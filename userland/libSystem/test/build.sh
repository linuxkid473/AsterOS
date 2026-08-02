#!/bin/bash
# Builds systest: a normal dynamically-linked executable proving real
# libSystem.B.dylib end to end (printf/malloc/fork/waitpid), the
# libSystem-equivalent of userland/dyld/test/build.sh's dyntest.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"
ROOT="$(cd ../../.. && pwd)"

CLANG=/Applications/Xcode-beta.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang
OUT="$ROOT/build/libSystem_obj"
mkdir -p "$OUT"

CFLAGS=(-target x86_64-apple-macos10.15 -ffreestanding -fno-stack-protector
        -fno-builtin -nostdlibinc -isystem "$ROOT/userland/libc/include"
        -O1 -g -Wall -Wextra -Wno-unused-parameter)

"$CLANG" "${CFLAGS[@]}" -c test_main.c -o "$OUT/systest_main.o"

# crt0.o/libc_start.o: the statically-linked-per-executable startup pair
# (see ../build.sh's comment on the crt1.o/libSystem split) -- reuse the
# ordinary non-PIC copies userland/libc/build.sh already produces, same
# objects busybox links against.
CRT0="$ROOT/build/libc_obj/crt0.o"
LIBC_START="$ROOT/build/libc_obj/libc_start.o"

# -no_pie, -bind_at_load: same reasoning as dyld/test/build.sh -- keeps
# this test isolated from main-executable ASLR, and dodges the host ld64
# crash building lazy-stub indirect symbol tables (it needs a real
# dyld_stub_binder export, which nothing in this project provides yet,
# real libSystem.B.dylib or not).
"$CLANG" -target x86_64-apple-macos10.15 -nostdlib -Wl,-no_pie -Wl,-bind_at_load -e _start \
	"$CRT0" "$LIBC_START" "$OUT/systest_main.o" "$OUT/libSystem.B.dylib" -o "$OUT/systest"

echo "built: $OUT/systest"
file "$OUT/systest"
otool -l "$OUT/systest" | grep -A3 "LC_LOAD_DYLINKER\|LC_LOAD_DYLIB"
