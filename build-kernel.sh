#!/bin/bash
# Builds the xnu-6153.141.1 kernel from src/ into build/kernel/kernel.development.
# Verified working end-to-end (Phase 2) from a clean BUILD/ dir on this host.
# See docs/architecture.md and patches/*.md for why each step below exists.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC="$ROOT/src"
SDKROOT="$ROOT/build/SDKs/MacOSX10.15.sdk"
TOOLS_BIN="$ROOT/build/tools/bin"
XCRUN_WRAPPER="$TOOLS_BIN/xcrun"

export PATH="$TOOLS_BIN:$PATH"

log() { printf '\n=== %s ===\n' "$1"; }

# --- Step 0: local SDK copy (only if missing) ---
if [ ! -d "$SDKROOT" ]; then
	log "Cloning local SDK copy"
	REAL_SDK=$(xcrun -sdk macosx --show-sdk-path)
	mkdir -p "$ROOT/build/SDKs"
	ditto "$REAL_SDK" "$SDKROOT"
fi

# --- Step 1: ctf tools (from dtrace) ---
if [ ! -x "$TOOLS_BIN/ctfmerge" ]; then
	log "Building ctf tools (ctfconvert, ctfdump, ctfmerge)"
	mkdir -p "$ROOT/build/dtrace/obj" "$ROOT/build/dtrace/sym" "$ROOT/build/dtrace/dst"
	(cd "$SRC/dtrace" && xcodebuild -project dtrace.xcodeproj \
		-target ctfconvert -target ctfdump -target ctfmerge \
		SDKROOT="$SDKROOT" SRCROOT="$(pwd)" \
		OBJROOT="$ROOT/build/dtrace/obj" SYMROOT="$ROOT/build/dtrace/sym" DSTROOT="$ROOT/build/dtrace/dst" \
		CODE_SIGNING_ALLOWED=NO CODE_SIGNING_REQUIRED=NO CODE_SIGN_IDENTITY="" \
		ARCHS=x86_64 ONLY_ACTIVE_ARCH=NO build)
	mkdir -p "$TOOLS_BIN"
	cp "$ROOT/build/dtrace/sym/Release/ctfconvert" \
	   "$ROOT/build/dtrace/sym/Release/ctfdump" \
	   "$ROOT/build/dtrace/sym/Release/ctfmerge" "$TOOLS_BIN/"
fi

# --- Step 2: AvailabilityVersions ---
if [ ! -f "$SDKROOT/usr/include/AvailabilityVersions.h" ]; then
	log "Installing AvailabilityVersions headers"
	(cd "$SRC/AvailabilityVersions" && make install SRCROOT="$(pwd)" OBJROOT=obj SYMROOT=sym DSTROOT="$SDKROOT" || true)
	(cd "$SRC/AvailabilityVersions" && make install_script SRCROOT="$(pwd)" DSTROOT="$SDKROOT")
fi

# --- Step 3: libplatform headers ---
if [ ! -f "$SDKROOT/usr/local/include/os/internal/internal_shared.h" ]; then
	log "Installing libplatform headers"
	LP="$SRC/libplatform"
	mkdir -p "$SDKROOT/usr/local/include/os/internal" "$SDKROOT/usr/local/include/libkern"
	cp -f "$LP/include/os/assumes.h" "$SDKROOT/usr/include/os/assumes.h"
	cp -f "$LP/internal/os/internal.h" "$LP/internal/os/internal_asm.h" "$LP/internal/os/yield.h" \
	      "$LP/private/os/internal/internal_shared.h" "$LP/private/os/internal/atomic.h" \
	      "$LP/private/os/internal/crashlog.h" "$SDKROOT/usr/local/include/os/internal/"
	cp -f "$LP/include/libkern/"*.h "$SDKROOT/usr/local/include/libkern/"
fi

# --- Step 4: xnu installhdrs (needed before libfirehose_kernel) ---
log "xnu installhdrs"
(cd "$SRC/xnu" && make SDKROOT="$SDKROOT" ARCH_CONFIGS=X86_64 XCRUN="$XCRUN_WRAPPER" installhdrs)

# --- Step 5: libfirehose_kernel (from libdispatch) ---
if [ ! -f "$SDKROOT/usr/local/lib/kernel/libfirehose_kernel.a" ]; then
	log "Building libfirehose_kernel"
	XNU_KFW="$SRC/xnu/BUILD/dst/System/Library/Frameworks/Kernel.framework"
	XNU_DST="$SRC/xnu/BUILD/dst"
	(cd "$SRC/libdispatch" && xcodebuild -project libdispatch.xcodeproj -target libfirehose_kernel \
		SDKROOT="$SDKROOT" SRCROOT="$(pwd)" \
		OBJROOT="$ROOT/build/libdispatch/obj" SYMROOT="$ROOT/build/libdispatch/sym" DSTROOT="$ROOT/build/libdispatch/dst" \
		CODE_SIGNING_ALLOWED=NO CODE_SIGNING_REQUIRED=NO CODE_SIGN_IDENTITY="" \
		ARCHS=x86_64 ONLY_ACTIVE_ARCH=NO VALID_ARCHS=x86_64 \
		HEADER_SEARCH_PATHS="\$(inherited) $XNU_KFW/PrivateHeaders $XNU_KFW/Headers $XNU_DST/usr/local/include/firehose $XNU_DST/usr/local/include/os" \
		build)
	mkdir -p "$SDKROOT/usr/local/lib/kernel"
	cp "$ROOT/build/libdispatch/sym/Release/libfirehose_kernel.a" \
	   "$ROOT/build/libdispatch/sym/Release/libfirehose_kernel_debug.a" \
	   "$ROOT/build/libdispatch/sym/Release/libfirehose_kernel_profile.a" \
	   "$SDKROOT/usr/local/lib/kernel/"
fi

# --- Step 6: also mirror the firehose private headers into the kernel's own -nostdinc path ---
mkdir -p "$SDKROOT/usr/local/include/kernel/os"
cp -f "$SRC/libdispatch/os/firehose_buffer_private.h" "$SRC/libdispatch/os/firehose_server_private.h" \
      "$SRC/libplatform/private/os/base_private.h" "$SDKROOT/usr/local/include/kernel/os/"

# --- Step 7: exporthdrs (own pass, before "all" -- see patches/) ---
log "xnu exporthdrs"
mkdir -p "$SRC/xnu/BUILD/obj/EXPORT_HDRS/libsa"
(cd "$SRC/xnu" && make SDKROOT="$SDKROOT" ARCH_CONFIGS=X86_64 KERNEL_CONFIGS=DEVELOPMENT XCRUN="$XCRUN_WRAPPER" exporthdrs)

# --- Step 8: the kernel itself ---
# SLIDE=0x10 (see makedefs/MakeInc.def: KERNEL_STATIC_SLIDE = SLIDE << 21)
# moves the kernel's static link/load address from the default
# 0xffffff8000200000 (phys 0x200000, i.e. 2MiB) up to 0xffffff8002200000
# (phys 0x2200000, ~34MiB). The default 2MiB address falls inside memory
# OVMF/QEMU's own EFI memory map has already claimed by the time our
# bootloader tries to AllocatePages(AllocateAddress, ...) there, so the
# load fails outright ("AllocatePages(segment) failed", boot dies before
# even reaching ExitBootServices). This was discovered and fixed earlier
# in the project but the SLIDE value was only ever passed ad hoc on the
# command line, so it was lost once the shell session that set it ended;
# baking it into the script here makes every future rebuild reproducible.
log "Building the kernel (this is the long step)"
(cd "$SRC/xnu" && make SDKROOT="$SDKROOT" ARCH_CONFIGS=X86_64 KERNEL_CONFIGS=DEVELOPMENT XCRUN="$XCRUN_WRAPPER" SLIDE=0x10 -j"$(sysctl -n hw.ncpu)")

mkdir -p "$ROOT/build/kernel"
cp "$SRC/xnu/BUILD/obj/DEVELOPMENT_X86_64/kernel.development" "$ROOT/build/kernel/"
log "Done: build/kernel/kernel.development"
file "$ROOT/build/kernel/kernel.development"
