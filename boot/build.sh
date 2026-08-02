#!/bin/bash
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"

CLANG=/Applications/Xcode-beta.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang
LLDLINK=/opt/homebrew/Cellar/llvm@17/17.0.6/bin/lld-link

CFLAGS="-target x86_64-unknown-windows -ffreestanding -fshort-wchar -mno-red-zone -Wall -Wextra -O1 -g"

"$CLANG" $CFLAGS -c boot.c -o boot.o
"$CLANG" -target x86_64-unknown-windows -c transition.S -o transition.o

"$LLDLINK" -subsystem:efi_application -entry:efi_main -nodefaultlib \
	boot.o transition.o -out:BOOTX64.EFI

echo "built: boot/BOOTX64.EFI"
file BOOTX64.EFI
