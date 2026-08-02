#!/bin/bash
# Cross-compiles neatvi against userland/libc, same recipe as
# userland/libc/build.sh / src/busybox/link_manual.sh: static, dyld-free
# Mach-O, no system headers/libs.
set -uo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"
ROOT="$(cd ../.. && pwd)"

CLANG=/Applications/Xcode-beta.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang
OUT="$ROOT/build/neatvi_obj"
mkdir -p "$OUT"

CFLAGS=(-target x86_64-apple-macos10.15 -ffreestanding -fno-stack-protector
        -fno-builtin -nostdlibinc -isystem "$ROOT/userland/libc/include"
        -O0 -g -Wall -Wno-format-truncation)

OBJS="vi ex lbuf mot sbuf ren dir syn reg led uc term rset rstr regex cmd tag conf"

fail=0
for base in $OBJS; do
	f="$base.c"
	echo "=== $f ==="
	if ! "$CLANG" "${CFLAGS[@]}" -c "$f" -o "$OUT/$base.o" 2>"$OUT/$base.err"; then
		fail=1
		echo "--- FAILED: $f ---"
		cat "$OUT/$base.err"
	fi
done

if [ "$fail" -ne 0 ]; then
	echo
	echo "=== BUILD FAILED, see errors above ==="
	exit 1
fi

echo "all objects built cleanly in $OUT"
