#!/bin/bash
# Builds libSystem.B.dylib: a real MH_DYLIB wrapping userland/libc/src, the
# thing every dynamically-linked executable now links against instead of
# going static. Replaces the empty placeholder dyld/mkrootfs.sh previously
# shipped at /usr/lib/libSystem.B.dylib just to satisfy the host ld64's
# "something must be named libSystem" requirement (see
# userland/dyld/test/build.sh) -- this is the real thing.
#
# Everything in libc/src goes into the dylib *except* crt0.S: unlike dyld
# (which must be its own self-contained, un-rebased image), libSystem is
# loaded and bound like any other dependency, so ordinary -fPIC with
# default (exported) visibility is correct and sufficient -- no
# -fvisibility=hidden dance needed.
#
# crt0.S (the _start trampoline) and libc_start.c (__libc_start itself,
# which calls this executable's own `main` directly) stay a separate,
# statically-linked-per-executable object pair -- Darwin's real
# crt1.o/libSystem split. Everything else in start.c (environ storage,
# atexit, exit/abort, __cxa_atexit/finalize) moves IN to the dylib:
# several other libc/src files (assert.c, stdlib_misc.c, syscalls.c)
# reference exit()/abort()/environ directly, and a dylib can't have those
# satisfied by a symbol living in whatever executable happens to link it
# later -- putting them in the same image sidesteps that entirely.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"
ROOT="$(cd ../.. && pwd)"
LIBC_SRC="$ROOT/userland/libc/src"

CLANG=/Applications/Xcode-beta.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang
OUT="$ROOT/build/libSystem_obj"
mkdir -p "$OUT"

CFLAGS=(-target x86_64-apple-macos10.15 -ffreestanding -fno-stack-protector
        -fno-builtin -fPIC -nostdlibinc -isystem "$ROOT/userland/libc/include"
        -O1 -g -Wall -Wextra -Wno-unused-parameter)

OBJS=()
for f in "$LIBC_SRC"/*.c "$LIBC_SRC"/*.S; do
	base=$(basename "${f%.*}")
	[ "$base" = "crt0" ] && continue
	[ "$base" = "libc_start" ] && continue
	"$CLANG" "${CFLAGS[@]}" -c "$f" -o "$OUT/$base.o"
	OBJS+=("$OUT/$base.o")
done

for f in "$LIBC_SRC"/musl_math/*.c; do
	base=$(basename "${f%.*}")
	"$CLANG" "${CFLAGS[@]}" -I "$LIBC_SRC/musl_math" -w -c "$f" -o "$OUT/$base.o"
	OBJS+=("$OUT/$base.o")
done

# Real Apple BlocksRuntime (_Block_copy/_Block_release, and the
# _NSConcreteStackBlock/_NSConcreteMallocBlock/_NSConcreteGlobalBlock data
# symbols clang's -fblocks codegen references directly by address for a
# block literal's `isa` field) -- same vendored source + config.h already
# proven to cross-compile clean for this exact target via
# userland/ld64_shim/build.sh, just linked into the real OS's libSystem
# now instead of only the host-side ld64 tool. Real Darwin ships these
# symbols as part of libSystem too, not a separate dylib.
BLOCKSRT_DIR="$ROOT/src/llvm-project/compiler-rt/lib/BlocksRuntime"
BLOCKSRT_CFLAGS=("${CFLAGS[@]}" -fblocks -I "$ROOT/userland/libSystem/blocksruntime_cfg" -Wno-deprecated-declarations)
for f in "$BLOCKSRT_DIR/runtime.c" "$BLOCKSRT_DIR/data.c"; do
	base=$(basename "${f%.*}")
	"$CLANG" "${BLOCKSRT_CFLAGS[@]}" -c "$f" -o "$OUT/$base.o"
	OBJS+=("$OUT/$base.o")
done

# The host's ld64 hard-refuses to link a dylib with itself, and also
# hard-refuses to build any dynamic Mach-O with an empty dependency list
# at all (see userland/dyld/build.sh's own placeholder for the first time
# this was hit) -- so libSystem.B.dylib, uniquely among everything we
# build, can't just link libSystem_stub.tbd (its install-name IS
# /usr/lib/libSystem.B.dylib, the self-link case). selflink_stub.tbd
# names something else instead; the real (tiny, empty) Mach-O it points
# to is built here and shipped by mkrootfs.sh so our own dyld's
# recursive dependency walk finds a real file when it loads *this*
# dylib's one dependency at runtime.
echo 'int __libsystem_selflink_stub__;' > "$OUT/selflink_stub_src.c"
"$CLANG" "${CFLAGS[@]}" -c "$OUT/selflink_stub_src.c" -o "$OUT/selflink_stub_src.o"
# This tiny placeholder hits the exact same "must link something" check,
# one level removed -- satisfied here with dyld's own libSystem_stub.tbd
# (install-name /usr/lib/libSystem.B.dylib). Not a real circular
# dependency at runtime: our dyld registers an image's path in g_images[]
# before recursing into its own deps (see find_loaded() in
# dyld/macho_load.c), so by the time this placeholder's nominal
# dependency on libSystem.B.dylib is walked, that image (already being
# loaded, one frame up) is simply found already-cached.
"$CLANG" -target x86_64-apple-macos10.15 -nostdlib -dynamiclib \
	-install_name /usr/lib/libSystem_selflink_stub.dylib \
	"$OUT/selflink_stub_src.o" "$ROOT/userland/dyld/libSystem_stub.tbd" -o "$OUT/libSystem_selflink_stub.dylib"

"$CLANG" -target x86_64-apple-macos10.15 -nostdlib -dynamiclib \
	-install_name /usr/lib/libSystem.B.dylib \
	"${OBJS[@]}" selflink_stub.tbd -o "$OUT/libSystem.B.dylib"

echo "built: $OUT/libSystem.B.dylib"
file "$OUT/libSystem.B.dylib" "$OUT/libSystem_selflink_stub.dylib"
