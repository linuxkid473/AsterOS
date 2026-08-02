#!/bin/bash
# Cross-compiles ld64 (apple-oss-distributions, ld64-530) as a static,
# no-dyld x86_64-apple-macos10.15 binary against userland/libc + our
# runtimes-install (libc++/libc++abi/libunwind), using the host-executable
# clang++ (build/llvm-host-build) as the cross compiler -- same pattern as
# userland/libc/build.sh and the static clang build.
set -uo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"
ROOT="$(cd ../.. && pwd)"
LD64_SRC="$ROOT/src/ld64/src"

CXX="$ROOT/build/llvm-host-build/bin/clang++"
CC="$ROOT/build/llvm-host-build/bin/clang"
OUT="$ROOT/build/ld64_obj"
mkdir -p "$OUT"

LIBCXX_INC="$ROOT/build/runtimes-install/include/c++/v1"
LIBC_INC="$ROOT/userland/libc/include"

INCLUDES=(
	-I "$LD64_SRC/ld"
	-I "$LD64_SRC/ld/parsers"
	-I "$LD64_SRC/ld/passes"
	-I "$LD64_SRC/abstraction"
	-I "$LD64_SRC/other"
	-I "$ROOT/userland/ld64_shim"
	-I "$ROOT/src/llvm-project/libunwind/include"
)

CXXFLAGS=(--target=x86_64-apple-macos10.15 -std=gnu++14
	-nostdinc++ -isystem "$LIBCXX_INC"
	-nostdlibinc -isystem "$LIBC_INC"
	"${INCLUDES[@]}"
	-D__DARWIN_ONLY_UNIX_CONFORMANCE=1
	-Wno-deprecated-declarations -Wno-unused-parameter -Wno-#warnings
	-fno-color-diagnostics -g -O0)

CFLAGS=(--target=x86_64-apple-macos10.15
	-nostdlibinc -isystem "$LIBC_INC"
	"${INCLUDES[@]}"
	-Wno-unused-parameter -fno-color-diagnostics -g -O0)

BLOCKSRT_DIR="$ROOT/src/llvm-project/compiler-rt/lib/BlocksRuntime"
BLOCKSRT_CFLAGS=("${CFLAGS[@]}" -I "$ROOT/userland/ld64_shim/blocksruntime_cfg" -Wno-deprecated-declarations)

FAIL=0
compile_one() {
	local f="$1" lang="$2"
	local base
	base=$(basename "${f%.*}")
	local out="$OUT/$base.o"
	if [ "$lang" = "cxx" ]; then
		"$CXX" "${CXXFLAGS[@]}" -c "$f" -o "$out" 2> "$OUT/$base.err"
	elif [ "$lang" = "blocksrt" ]; then
		"$CC" "${BLOCKSRT_CFLAGS[@]}" -c "$f" -o "$out" 2> "$OUT/$base.err"
	else
		"$CC" "${CFLAGS[@]}" -c "$f" -o "$out" 2> "$OUT/$base.err"
	fi
	if [ $? -ne 0 ]; then
		echo "FAIL: $f"
		FAIL=1
	else
		echo "ok:   $f"
		rm -f "$OUT/$base.err"
	fi
}

# Real ld64 sources (excluding textstub_dylib_file.cpp/lto_file.cpp --
# real libtapi/.tbd + libLTO.dylib support, both replaced by our shim --
# and passes/bitcode_bundle.cpp -- real xar/bitcode-bundle support, not
# needed since our clang never emits LLVM bitcode).
CXX_SOURCES=(
	"$LD64_SRC/ld/InputFiles.cpp"
	"$LD64_SRC/ld/ld.cpp"
	"$LD64_SRC/ld/LinkEdit.hpp"
	"$LD64_SRC/ld/Options.cpp"
	"$LD64_SRC/ld/OutputFile.cpp"
	"$LD64_SRC/ld/PlatformSupport.cpp"
	"$LD64_SRC/ld/Resolver.cpp"
	"$LD64_SRC/ld/Snapshot.cpp"
	"$LD64_SRC/ld/SymbolTable.cpp"
	"$LD64_SRC/ld/code-sign-blobs/blob.cpp"
	"$LD64_SRC/ld/parsers/archive_file.cpp"
	"$LD64_SRC/ld/parsers/macho_dylib_file.cpp"
	"$LD64_SRC/ld/parsers/macho_relocatable_file.cpp"
	"$LD64_SRC/ld/parsers/opaque_section_file.cpp"
	"$LD64_SRC/ld/passes/branch_island.cpp"
	"$LD64_SRC/ld/passes/branch_shim.cpp"
	"$LD64_SRC/ld/passes/code_dedup.cpp"
	"$LD64_SRC/ld/passes/compact_unwind.cpp"
	"$LD64_SRC/ld/passes/dtrace_dof.cpp"
	"$LD64_SRC/ld/passes/dylibs.cpp"
	"$LD64_SRC/ld/passes/got.cpp"
	"$LD64_SRC/ld/passes/huge.cpp"
	"$LD64_SRC/ld/passes/inits.cpp"
	"$LD64_SRC/ld/passes/objc.cpp"
	"$LD64_SRC/ld/passes/order.cpp"
	"$LD64_SRC/ld/passes/stubs/stubs.cpp"
	"$LD64_SRC/ld/passes/thread_starts.cpp"
	"$LD64_SRC/ld/passes/tlvp.cpp"
	"$ROOT/userland/ld64_shim/lto_stub.cpp"
	"$ROOT/userland/ld64_shim/tapi_stub.cpp"
	"$ROOT/userland/ld64_shim/passes_stub.cpp"
)

C_SOURCES=(
	"$LD64_SRC/ld/debugline.c"
	"$ROOT/userland/ld64_shim/cc_md5.c"
	"$ROOT/userland/ld64_shim/vers.c"
)

for f in "${CXX_SOURCES[@]}"; do
	[[ "$f" == *.hpp ]] && continue
	compile_one "$f" cxx
done

for f in "${C_SOURCES[@]}"; do
	compile_one "$f" c
done

compile_one "$BLOCKSRT_DIR/runtime.c" blocksrt
compile_one "$BLOCKSRT_DIR/data.c" blocksrt

echo "======================================"
if [ $FAIL -ne 0 ]; then
	echo "BUILD FAILED -- see $OUT/*.err"
else
	echo "all files compiled OK"
fi
