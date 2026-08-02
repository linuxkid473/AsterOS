#!/bin/bash
# Compiles the libc shim's own sources to object files, standalone (no
# busybox involved) -- a fast smoke test that the shim itself is
# self-consistent before pulling in busybox's much larger source tree.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"
ROOT="$(cd ../.. && pwd)"

CLANG=/Applications/Xcode-beta.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang
OUT="$ROOT/build/libc_obj"
mkdir -p "$OUT"

CFLAGS=(-target x86_64-apple-macos10.15 -ffreestanding -fno-stack-protector
        -fno-builtin -nostdlibinc -isystem "$ROOT/userland/libc/include"
        -O1 -g -Wall -Wextra -Wno-unused-parameter)

for f in src/*.c src/*.S; do
	base=$(basename "${f%.*}")
	echo "=== $f ==="
	"$CLANG" "${CFLAGS[@]}" -c "$f" -o "$OUT/$base.o"
done

# Vendored musl libm (MIT-licensed, see src/musl_math/COPYRIGHT) --
# needs its own include path for libm.h/the *_data.h tables, and -w
# since it's upstream code we don't want to edit just to silence our
# own -Wall/-Wextra.
for f in src/musl_math/*.c; do
	base=$(basename "${f%.*}")
	echo "=== $f ==="
	"$CLANG" "${CFLAGS[@]}" -I src/musl_math -w -c "$f" -o "$OUT/$base.o"
done

echo "libc objects built in $OUT:"
ls -la "$OUT"
