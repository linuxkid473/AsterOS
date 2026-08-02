#!/bin/bash
# Assembles boot/fat16.img, the FAT16 image bsd/miscfs/fat16lite mounts as
# root. Reformatted from scratch every time rather than mcopy -o'd onto an
# existing image -- repeated in-place overwrites fragment the FAT cluster
# chain, which fat16lite's pager_map_to_phys_contiguous can't handle (see
# TODO.md Phase 9 item 3).
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."

ROOTFS_IMG="boot/fat16.img"
ROOTFS_SIZE_MB=220

CLANG_BIN="build/llvm-static-build/bin/clang"
LD_BIN="build/ld64_bin/ld64"
LIBCXX="build/runtimes-install/lib/libc++.a"
LIBCXXABI="build/runtimes-install/lib/libc++abi.a"
LIBUNWIND="build/runtimes-install/lib/libunwind.a"
CLANGRT="build/compiler-rt-install/lib/darwin/libclang_rt.osx.a"
CLANG_RESOURCE_INCLUDE="build/llvm-static-build/lib/clang/20/include"

rm -f "$ROOTFS_IMG"
dd if=/dev/zero of="$ROOTFS_IMG" bs=1m count="$ROOTFS_SIZE_MB" status=none
# Geometry (8 sectors/cluster, 8 reserved sectors, 32-sector root dir) matches
# the hand-built image fat16lite was originally verified against -- letting
# mformat pick its own defaults here produced a layout where a file's data
# start wasn't page-aligned, which pager_map_to_phys_contiguous requires
# (panics with "computed address ... is not page-aligned" otherwise).
mformat -i "$ROOTFS_IMG" -R 8 -c 8 -r 32 -h 16 -n 63 -v ROOTFS ::

for d in bin sbin dev etc tmp usr var; do
	mmd -i "$ROOTFS_IMG" "::/$d"
done
mmd -i "$ROOTFS_IMG" ::/usr/lib
mmd -i "$ROOTFS_IMG" ::/var/log

mcopy -i "$ROOTFS_IMG" src/busybox/busybox_unstripped ::/bin/busybox
mcopy -i "$ROOTFS_IMG" build/launchd/launchd ::/sbin/launchd

mmd -i "$ROOTFS_IMG" ::/etc/launchd
mmd -i "$ROOTFS_IMG" ::/etc/launchd/daemons
mcopy -i "$ROOTFS_IMG" userland/launchd/daemons/com.asteros.shell.plist ::/etc/launchd/daemons/com.asteros.shell.plist
mcopy -i "$ROOTFS_IMG" userland/launchd/daemons/com.asteros.echotest.plist ::/etc/launchd/daemons/com.asteros.echotest.plist
if [ -f build/libSystem_obj/pthreadtest ]; then
	mcopy -i "$ROOTFS_IMG" userland/launchd/daemons/com.asteros.pthreadtest.plist ::/etc/launchd/daemons/com.asteros.pthreadtest.plist
fi
if [ -f build/launchd_test/echotest ]; then
	echo "echotest found in build/ -- including it in the rootfs"
	mcopy -i "$ROOTFS_IMG" build/launchd_test/echotest ::/bin/echotest
fi
if [ -f build/corefoundation_obj/cftest ]; then
	mcopy -i "$ROOTFS_IMG" userland/launchd/daemons/com.asteros.cftest.plist ::/etc/launchd/daemons/com.asteros.cftest.plist
fi

DYLD_BIN="build/dyld_obj/dyld"
LIBSYSTEM_REAL="build/libSystem_obj/libSystem.B.dylib"
LIBSYSTEM_PLACEHOLDER="build/dyld_obj/libSystem.B.dylib"
if [ -f "$DYLD_BIN" ]; then
	echo "dyld found in build/ -- including it in the rootfs"
	mcopy -i "$ROOTFS_IMG" "$DYLD_BIN" ::/usr/lib/dyld
	if [ -f "$LIBSYSTEM_REAL" ]; then
		echo "real libSystem.B.dylib found in build/ -- including it in the rootfs"
		mcopy -i "$ROOTFS_IMG" "$LIBSYSTEM_REAL" ::/usr/lib/libSystem.B.dylib
		mcopy -i "$ROOTFS_IMG" build/libSystem_obj/libSystem_selflink_stub.dylib ::/usr/lib/libSystem_selflink_stub.dylib
		if [ -f build/libSystem_obj/systest ]; then
			mcopy -i "$ROOTFS_IMG" build/libSystem_obj/systest ::/bin/systest
		fi
		if [ -f build/libSystem_obj/pthreadtest ]; then
			mcopy -i "$ROOTFS_IMG" build/libSystem_obj/pthreadtest ::/bin/pthreadtest
		fi
	elif [ -f "$LIBSYSTEM_PLACEHOLDER" ]; then
		mcopy -i "$ROOTFS_IMG" "$LIBSYSTEM_PLACEHOLDER" ::/usr/lib/libSystem.B.dylib
	fi
	if [ -f build/dyld_obj/libtest.dylib ] && [ -f build/dyld_obj/dyntest ]; then
		mcopy -i "$ROOTFS_IMG" build/dyld_obj/libtest.dylib ::/usr/lib/libtest.dylib
		mcopy -i "$ROOTFS_IMG" build/dyld_obj/dyntest ::/bin/dyntest
	fi
	if [ -f build/libobjc_obj/libobjc.A.dylib ] && [ -f build/libobjc_obj/objctest ]; then
		echo "libobjc found in build/ -- including it in the rootfs"
		mcopy -i "$ROOTFS_IMG" build/libobjc_obj/libobjc.A.dylib ::/usr/lib/libobjc.A.dylib
		mcopy -i "$ROOTFS_IMG" build/libobjc_obj/objctest ::/bin/objctest
	fi
	if [ -f build/corefoundation_obj/libCoreFoundation.dylib ] && [ -f build/corefoundation_obj/cftest ]; then
		echo "CoreFoundation found in build/ -- including it in the rootfs"
		mcopy -i "$ROOTFS_IMG" build/corefoundation_obj/libCoreFoundation.dylib ::/usr/lib/libCoreFoundation.dylib
		mcopy -i "$ROOTFS_IMG" build/corefoundation_obj/cftest ::/bin/cftest
	fi
fi

if [ -x "$CLANG_BIN" ] && [ -x "$LD_BIN" ] && [ -f "$LIBCXX" ] && [ -f "$LIBCXXABI" ] \
    && [ -f "$LIBUNWIND" ] && [ -f "$CLANGRT" ]; then
	echo "native toolchain found in build/ -- including it in the rootfs"

	# libc.a is just an archive of the already-built libc objects -- cheap
	# to (re)create here, not a rebuild of anything.
	ar rcs build/libc_obj/libc.a build/libc_obj/*.o

	mmd -i "$ROOTFS_IMG" ::/usr/bin
	mmd -i "$ROOTFS_IMG" ::/usr/include

	mcopy -i "$ROOTFS_IMG" "$CLANG_BIN" ::/usr/bin/clang
	mcopy -i "$ROOTFS_IMG" "$LD_BIN" ::/usr/bin/ld
	mcopy -i "$ROOTFS_IMG" userland/toolchain/clang.cfg ::/usr/bin/clang.cfg
	mcopy -i "$ROOTFS_IMG" build/neatvi_obj/neatvi ::/usr/bin/neatvi

	mcopy -i "$ROOTFS_IMG" build/libc_obj/libc.a ::/usr/lib/libc.a
	mcopy -i "$ROOTFS_IMG" "$LIBCXX" ::/usr/lib/libcxx.a
	mcopy -i "$ROOTFS_IMG" "$LIBCXXABI" ::/usr/lib/libcxxab.a
	mcopy -i "$ROOTFS_IMG" "$LIBUNWIND" ::/usr/lib/libunwnd.a
	mcopy -i "$ROOTFS_IMG" "$CLANGRT" ::/usr/lib/clangrt.a

	mcopy -s -i "$ROOTFS_IMG" userland/libc/include/* ::/usr/include/
	mmd -i "$ROOTFS_IMG" ::/usr/lib/clang
	mmd -i "$ROOTFS_IMG" ::/usr/lib/clang/20
	mcopy -s -i "$ROOTFS_IMG" "$CLANG_RESOURCE_INCLUDE" ::/usr/lib/clang/20/
else
	echo "no prebuilt native toolchain in build/ -- deploying core rootfs only"
	mcopy -i "$ROOTFS_IMG" build/neatvi_obj/neatvi ::/bin/neatvi
fi

echo "rootfs assembled: $ROOTFS_IMG"
mdir -i "$ROOTFS_IMG" ::
