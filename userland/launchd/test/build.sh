#!/bin/bash
# Builds echotest, the standalone KeepAlive regression daemon -- same
# static -nostdlib -e _start recipe as the launchd Makefile target
# itself, just built outside the top-level Makefile like the other
# test/ binaries in this project (dyntest, systest, objctest).
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"
ROOT="$(cd ../../.. && pwd)"

CLANG=/Applications/Xcode-beta.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang
OUT="$ROOT/build/launchd_test"
mkdir -p "$OUT"

CFLAGS=(-target x86_64-apple-macos10.15 -ffreestanding -fno-stack-protector
        -fno-builtin -nostdlibinc -isystem "$ROOT/userland/libc/include"
        -O1 -g -Wall -Wextra)

"$CLANG" "${CFLAGS[@]}" -c echotest.c -o "$OUT/echotest.o"

"$CLANG" -target x86_64-apple-macos10.15 -nostdlib -static -e _start \
	"$OUT/echotest.o" "$ROOT"/build/libc_obj/*.o -o "$OUT/echotest"

echo "built: $OUT/echotest"
file "$OUT/echotest"
