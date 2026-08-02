#!/bin/bash
# Builds the dyld end-to-end smoke test: libtest.dylib (one exported
# function + one exported data symbol) and dyntest, a normal libc-based
# executable that dynamically links against it.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"
ROOT="$(cd ../../.. && pwd)"

CLANG=/Applications/Xcode-beta.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang
OUT="$ROOT/build/dyld_obj"
mkdir -p "$OUT"

CFLAGS=(-target x86_64-apple-macos10.15 -ffreestanding -fno-stack-protector
        -fno-builtin -nostdlibinc -isystem "$ROOT/userland/libc/include"
        -O1 -g -Wall -Wextra -Wno-unused-parameter)

"$CLANG" "${CFLAGS[@]}" -c libtest.c -o "$OUT/libtest.o"
"$CLANG" "${CFLAGS[@]}" -c test_main.c -o "$OUT/test_main.o"

# The host's modern ld64 hard-refuses any dynamic executable/dylib that
# doesn't link *something* named libSystem -- unavoidable even for an
# otherwise-empty dylib (confirmed empirically: building a placeholder
# libSystem itself hits the same check). We have no libSystem and never
# will, so we hand it a hand-authored stub .tbd (link-time-only, no
# symbols) purely to satisfy the bookkeeping; see mkrootfs.sh for the
# real (also-empty) Mach-O placeholder dyld actually loads on Asteros.
STUB_TBD="$ROOT/userland/dyld/libSystem_stub.tbd"

# install_name is what a client's LC_LOAD_DYLIB ends up embedding when
# linked against this .dylib (regardless of the host path we hand ld64
# below) -- must match where mkrootfs.sh actually places it.
"$CLANG" -target x86_64-apple-macos10.15 -nostdlib -dynamiclib \
	-install_name /usr/lib/libtest.dylib \
	"$OUT/libtest.o" "$STUB_TBD" -o "$OUT/libtest.dylib"

LIBC_OBJS=("$ROOT"/build/libc_obj/*.o)

# Deliberately non-PIE (-Wl,-no_pie): isolates dyld's own rebase/bind
# logic from also being the first exercise of this kernel's ASLR slide
# computation. Not -static, so ld64 defaults to LC_LOAD_DYLINKER=/usr/lib/dyld.
#
# -bind_at_load: the host's ld64 segfaults building the indirect symbol
# table for lazy stubs when there's no real libSystem providing
# dyld_stub_binder (IndirectSymbolTableBuilderImpl hits an internal
# "_sideInfo" assertion in Atom.h -- a host linker limitation, not
# something wrong with our bind opcode stream). -bind_at_load makes ld64
# emit ordinary eager BIND_OPCODE entries for dyntest_add instead of a
# stub_helper/dyld_stub_binder call, sidestepping the crash. Our
# dyld_stub_binder trampoline (stub_binder.c/.S) is still real and
# correct, just unexercised by this test until we have a linker that can
# actually emit lazy stubs without libSystem.
"$CLANG" -target x86_64-apple-macos10.15 -nostdlib -Wl,-no_pie -Wl,-bind_at_load -e _start \
	"${LIBC_OBJS[@]}" "$OUT/test_main.o" "$OUT/libtest.dylib" -o "$OUT/dyntest"

echo "built: $OUT/libtest.dylib and $OUT/dyntest"
file "$OUT/libtest.dylib" "$OUT/dyntest"
otool -l "$OUT/dyntest" | grep -A3 "LC_LOAD_DYLINKER\|LC_LOAD_DYLIB"
