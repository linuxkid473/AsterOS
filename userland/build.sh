#!/bin/bash
# Builds the tiny init/shell as a static, no-dyld Mach-O executable.
#
# Confirmed empirically (see TODO.md Phase 5) that xnu's mach_loader.c only
# sets result->needs_dynlinker under CONFIG_EMBEDDED (iOS-style builds,
# compiled out for our x86_64 desktop-style kernel) -- so a plain
# `-static -nostdlib` MH_EXECUTE with no LC_LOAD_DYLINKER loads with zero
# dyld involvement, using the exact same standard-linker recipe as any other
# static executable (no custom Mach-O construction needed, unlike the EFI
# bootloader which had to avoid ld64 entirely).
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"

CLANG=/Applications/Xcode-beta.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang

CFLAGS="-target x86_64-apple-macos10.15 -ffreestanding -fno-stack-protector -fno-builtin -O1 -g -Wall -Wextra"

"$CLANG" $CFLAGS -c start.S -o start.o
"$CLANG" $CFLAGS -c shell.c -o shell.o

"$CLANG" -target x86_64-apple-macos10.15 -nostdlib -static -e _start \
	start.o shell.o -o launchd

echo "built: userland/launchd"
file launchd
otool -hv launchd
