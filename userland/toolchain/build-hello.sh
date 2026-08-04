#!/bin/sh
# Compiles and runs /tmp/hello.m entirely on-target: the native clang/ld
# installed at /usr/bin (Phase 10) against the Foundation SDK headers
# installed at /usr/include (CoreFoundation + Foundation), dynamically
# linked against the real /usr/lib/lib{Foundation,CoreFoundation,objc,
# System.B}.dylib. --no-default-config skips /usr/bin/clang.cfg, which
# is set up for *static* hello-worlds (-static + libc.a/libcxx.a etc.) --
# incompatible with dynamically linking Foundation, so this passes every
# flag explicitly instead, same recipe as Foundation/test/build.sh's
# dynamic link (crt0.o + libc_start.o statically bring in _start, every
# other symbol resolved against the dylibs at runtime by dyld).
set -e

clang --no-default-config --target=x86_64-apple-macos10.15 \
	-fobjc-runtime=macosx -fobjc-arc \
	-nostdlibinc -isystem /usr/include \
	-c /tmp/hello.m -o /tmp/hello.o

# Full paths, not -lFoundation/-lobjc/-lSystem: fat16 has no symlinks
# (see mkrootfs.sh), so there's no libobjc.dylib/libSystem.dylib link-name
# alias to the real libobjc.A.dylib/libSystem.B.dylib on disk for -l's
# lib<name>.dylib search to find.
clang --no-default-config --target=x86_64-apple-macos10.15 \
	-nostdlib -fuse-ld=/usr/bin/ld \
	-Wl,-no_pie -Wl,-bind_at_load -e _start \
	/usr/lib/crt0.o /usr/lib/libc_start.o /tmp/hello.o \
	/usr/lib/libFoundation.dylib /usr/lib/libCoreFoundation.dylib \
	/usr/lib/libobjc.A.dylib /usr/lib/libSystem.B.dylib \
	-o /tmp/hello

echo "build ok, running:"
/tmp/hello
