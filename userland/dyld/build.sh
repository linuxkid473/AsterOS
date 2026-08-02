#!/bin/bash
# Builds dyld itself: an MH_DYLINKER Mach-O.
#
# Ground-truthed empirically (not just assumed): mach_loader.c's
# load_dylinker() unconditionally computes and applies an ASLR slide to
# the dylinker regardless of the MH_PIE bit or a -no_pie link -- an
# earlier version of this script tried to dodge that by forcing a fixed
# non-PIE load address, but the kernel slides it anyway, and non-PIE
# codegen is free to bake in absolute addresses for globals that break
# under a real slide. Real dyld avoids this by being pure position-
# independent code (every reference RIP-relative, correct at any load
# address with zero fixups of its own needed) rather than by fixing its
# address -- hence -fPIC below, on every object including the libc
# pieces we link in. Since dyld has no dylib dependencies of its own,
# `-fPIC` only changes codegen (absolute -> RIP-relative for globals);
# there's no GOT/PLT indirection actually needed at runtime for symbols
# that resolve within this one statically-linked image.
#
# We compile our own PIC copies of the libc pieces dyld needs (syscalls/
# string/malloc) straight from userland/libc/src rather than reusing
# build/libc_obj/*.o, which is deliberately built non-PIC for everything
# else in this project.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"
ROOT="$(cd ../.. && pwd)"

CLANG=/Applications/Xcode-beta.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang
OUT="$ROOT/build/dyld_obj"
mkdir -p "$OUT"

# -fvisibility=hidden matters for correctness, not just symbol hygiene:
# without it, clang treats our externally-linked globals (g_images, etc.)
# as preemptible under -fPIC and routes references through a GOT slot
# that only a rebase pass would fix up -- and nothing ever rebases dyld's
# own image (it's the one providing rebase to everyone else). Hidden
# visibility tells it nothing outside this link unit can ever reference
# or override these symbols, so it addresses them directly (RIP-relative)
# instead, which is correct at any load address with no fixups needed.
CFLAGS=(-target x86_64-apple-macos10.15 -ffreestanding -fno-stack-protector
        -fno-builtin -fPIC -fvisibility=hidden -nostdlibinc
        -isystem "$ROOT/userland/libc/include"
        -O1 -g -Wall -Wextra -Wno-unused-parameter)

for f in *.c *.S; do
	base=$(basename "${f%.*}")
	"$CLANG" "${CFLAGS[@]}" -c "$f" -o "$OUT/$base.o"
done

for f in syscalls string malloc; do
	"$CLANG" "${CFLAGS[@]}" -c "$ROOT/userland/libc/src/$f.c" -o "$OUT/libc_pic_$f.o"
done
LIBC_OBJS=("$OUT/libc_pic_syscalls.o" "$OUT/libc_pic_string.o" "$OUT/libc_pic_malloc.o")

DYLD_OBJS=(dyld_start.o dyld_main.o macho_load.o rebase.o bind.o export_trie.o
           stub_binder.o stub_binder_asm.o dyld_panic.o libc_shims.o)

"$CLANG" -target x86_64-apple-macos10.15 -nostdlib \
	-Wl,-dylinker -Wl,-dylinker_install_name,/usr/lib/dyld \
	-e _dyld_start \
	"${DYLD_OBJS[@]/#/$OUT/}" "${LIBC_OBJS[@]}" -o "$OUT/dyld"

echo "built: $OUT/dyld"
file "$OUT/dyld"
otool -hv "$OUT/dyld"
otool -l "$OUT/dyld" | grep -A2 LC_ID_DYLINKER

# The host's ld64 hard-refuses to build ANY dynamic executable or dylib
# that doesn't link something named libSystem -- confirmed empirically,
# applies even to an otherwise-empty dylib, so it can't be satisfied by
# building our own placeholder the normal way (that build would hit the
# very same check). libSystem_stub.tbd is a hand-authored, link-time-only
# stub (no real symbols) that satisfies the bookkeeping; every
# dynamically-linked Asteros binary ends up with an LC_LOAD_DYLIB for
# "/usr/lib/libSystem.B.dylib" as a result, so we ship a real (separately
# named, to avoid ld64's "can't link a dylib with itself" check) empty
# Mach-O dylib at that runtime path -- our dyld loads it like any other
# dependency, it just has nothing dyld_stub_binder-special-cased code
# ever needs to call into.
echo 'int __libsystem_placeholder__;' > "$OUT/libsystem_placeholder.c"
"$CLANG" "${CFLAGS[@]}" -c "$OUT/libsystem_placeholder.c" -o "$OUT/libsystem_placeholder.o"
"$CLANG" -target x86_64-apple-macos10.15 -nostdlib -dynamiclib \
	-install_name /usr/lib/libSystem_placeholder_self.dylib \
	"$OUT/libsystem_placeholder.o" libSystem_stub.tbd -o "$OUT/libSystem.B.dylib"
echo "built: $OUT/libSystem.B.dylib (placeholder)"
