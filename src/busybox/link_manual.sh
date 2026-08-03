#!/bin/bash
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")"
ROOT="$(cd ../.. && pwd)"

CLANG=/Applications/Xcode-beta.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang

LIBC_OBJS=("$ROOT"/build/libc_obj/*.o)

BB_OBJS=(
	applets/built-in.o
	archival/lib.a archival/libarchive/lib.a
	console-tools/lib.a
	coreutils/lib.a coreutils/libcoreutils/lib.a
	debianutils/lib.a
	klibc-utils/lib.a
	e2fsprogs/lib.a
	editors/lib.a
	findutils/lib.a
	init/lib.a
	libbb/lib.a
	libpwdgrp/lib.a
	loginutils/lib.a
	mailutils/lib.a
	miscutils/lib.a
	modutils/lib.a
	networking/lib.a networking/libiproute/lib.a networking/udhcp/lib.a
	printutils/lib.a
	procps/lib.a
	runit/lib.a
	selinux/lib.a
	shell/lib.a
	sysklogd/lib.a
	util-linux/lib.a util-linux/volume_id/lib.a
	archival/built-in.o archival/libarchive/built-in.o
	console-tools/built-in.o
	coreutils/built-in.o coreutils/libcoreutils/built-in.o
	debianutils/built-in.o
	klibc-utils/built-in.o
	e2fsprogs/built-in.o
	editors/built-in.o
	findutils/built-in.o
	init/built-in.o
	libbb/built-in.o
	libpwdgrp/built-in.o
	loginutils/built-in.o
	mailutils/built-in.o
	miscutils/built-in.o
	modutils/built-in.o
	networking/built-in.o networking/libiproute/built-in.o networking/udhcp/built-in.o
	printutils/built-in.o
	procps/built-in.o
	runit/built-in.o
	selinux/built-in.o
	shell/built-in.o
	sysklogd/built-in.o
	util-linux/built-in.o util-linux/volume_id/built-in.o
)

"$CLANG" -target x86_64-apple-macos10.15 -nostdlib -static -e _start \
	"${LIBC_OBJS[@]}" "${BB_OBJS[@]}" -o busybox_unstripped.new

# cmp-before-mv, same reason as build-kernel.sh's kernel.development copy:
# the objects above are already incrementally rebuilt by busybox's own
# Kbuild, but this final link step re-runs unconditionally -- only replace
# the real output (and its mtime) when the link result actually changed,
# so the top-level Makefile's $(ROOTFS_IMG) rule can skip the (expensive,
# always-from-scratch) image assembly when nothing did.
if ! cmp -s busybox_unstripped.new busybox_unstripped 2>/dev/null; then
	mv busybox_unstripped.new busybox_unstripped
else
	rm -f busybox_unstripped.new
fi

echo "linked: busybox_unstripped"
file busybox_unstripped
