# Top-level build orchestration for AsterOS.
# `make run` builds everything it needs and boots the result in QEMU.
# See docs/architecture.md / TODO.md for what each piece actually is.

ROOT        := $(abspath .)
CLANG       := /Applications/Xcode-beta.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang
TOOLS_BIN   := $(ROOT)/build/tools/bin
NPROC       := $(shell sysctl -n hw.ncpu)
export PATH := $(TOOLS_BIN):$(PATH)

QEMU        := qemu-system-x86_64
OVMF_CODE   := boot/OVMF_CODE.fd
OVMF_VARS   := boot/OVMF_VARS.fd
ESP_IMG     := boot/esp.img
ROOTFS_IMG  := boot/fat16.img

KERNEL_BIN  := build/kernel/kernel.development
BOOTX64     := boot/BOOTX64.EFI
LIBC_STAMP  := build/libc_obj/.stamp
BUSYBOX_BIN := src/busybox/busybox_unstripped
NEATVI_BIN  := build/neatvi_obj/neatvi
INIT_BIN    := build/init_launcher/init

TOOLCHAIN_CLANG := build/llvm-static-build/bin/clang
TOOLCHAIN_LD    := build/ld64_bin/ld64

.PHONY: all kernel bootloader libc busybox neatvi init toolchain image run clean help

all: image

help:
	@echo "targets: all, kernel, bootloader, libc, busybox, neatvi, init, toolchain, image, run, clean"

# --- kernel -----------------------------------------------------------
# Always delegates to build-kernel.sh, which itself calls into xnu's own
# incremental `make` -- fast (a no-op check) when nothing under src/xnu
# changed, so there's no need to duplicate that staleness tracking here.
kernel:
	./build-kernel.sh

# --- bootloader ---------------------------------------------------------
bootloader: $(BOOTX64)
$(BOOTX64): boot/boot.c boot/darwin_boot.h boot/mach_o.h boot/efi.h boot/transition.S boot/build.sh
	cd boot && bash build.sh

# --- libc shim ----------------------------------------------------------
libc: $(LIBC_STAMP)
$(LIBC_STAMP): $(wildcard userland/libc/src/*.c userland/libc/src/*.S userland/libc/src/musl_math/*.c) \
               $(wildcard userland/libc/include/*.h) userland/libc/build.sh
	cd userland/libc && bash build.sh
	touch $@

# --- busybox --------------------------------------------------------------
# Always delegates to busybox's own Kbuild, which is genuinely incremental
# (only recompiles changed .c files) -- fast when nothing changed. Its own
# trylink-based final link doesn't know about our custom libc object set
# (see link_manual.sh), so that last step is expected to fail; everything
# before it (every lib.a/built-in.o) still builds, which is all
# link_manual.sh actually needs, and that final link itself is a single
# fast clang invocation regardless.
busybox: $(LIBC_STAMP)
	$(MAKE) -C src/busybox CC="$(TOOLS_BIN)/cc-nogroup" AR="$(TOOLS_BIN)/ar" -j$(NPROC) || true
	cd src/busybox && ROOT="$(ROOT)" bash link_manual.sh

# --- neatvi ---------------------------------------------------------------
neatvi: $(NEATVI_BIN)
$(NEATVI_BIN): $(wildcard src/neatvi/*.c src/neatvi/*.h) $(LIBC_STAMP)
	cd src/neatvi && bash build.sh
	cd src/neatvi && bash link.sh

# --- init (PID 1) -----------------------------------------------------
init: $(INIT_BIN)
$(INIT_BIN): userland/init_launcher.c $(LIBC_STAMP)
	mkdir -p build/init_launcher
	$(CLANG) -target x86_64-apple-macos10.15 -ffreestanding -fno-stack-protector -fno-builtin \
	    -nostdlibinc -isystem userland/libc/include -O1 -g \
	    -c userland/init_launcher.c -o build/init_launcher/init_launcher.o
	$(CLANG) -target x86_64-apple-macos10.15 -nostdlib -static -e _start \
	    build/init_launcher/init_launcher.o build/libc_obj/*.o -o $(INIT_BIN)

# --- native toolchain (never built here -- see docs/architecture.md) ------
# LLVM/clang/ld64 are cross-built by hand over many hours (Phase 10 in
# TODO.md); this Makefile only ever *packages* whatever's already sitting
# in build/ (userland/mkrootfs.sh does the actual copying into the image).
toolchain:
	@if [ -x "$(TOOLCHAIN_CLANG)" ] && [ -x "$(TOOLCHAIN_LD)" ]; then \
		echo "prebuilt native toolchain present in build/ -- image will include it"; \
	else \
		echo "no prebuilt native toolchain in build/ -- image will be core-only"; \
		echo "(building one from scratch is a separate, long-running process -- see TODO.md Phase 10)"; \
	fi

# --- disk images ------------------------------------------------------
image: $(ESP_IMG)

$(ROOTFS_IMG): busybox $(NEATVI_BIN) $(INIT_BIN)
	bash userland/mkrootfs.sh

$(ESP_IMG): $(BOOTX64) kernel $(ROOTFS_IMG)
	bash boot/mkesp.sh

# --- run ----------------------------------------------------------------
# -serial mon:stdio multiplexes the serial console and the QEMU monitor
# onto the same terminal (Ctrl-A C toggles to the monitor and back) --
# this is where kernel -v boot messages (kprintf) land. The interactive
# shell itself lives on the QEMU window's own display (GOP framebuffer
# console, serial=2 in boot.c's cmdline is input-only) -- that window is
# where you actually type commands, not the terminal running this target.
run: image
	$(QEMU) -machine q35 -cpu Haswell -m 2048 \
	    -drive if=pflash,format=raw,readonly=on,file=$(OVMF_CODE) \
	    -drive if=pflash,format=raw,file=$(OVMF_VARS) \
	    -drive format=raw,file=$(ESP_IMG) \
	    -serial mon:stdio -no-reboot -no-shutdown

# --- clean ----------------------------------------------------------------
# Removes this Makefile's own products so `make`/`make run` regenerates
# them from scratch. Leaves build/SDKs, the xnu BUILD/ tree, and any
# built native toolchain alone -- those are expensive prerequisites this
# Makefile doesn't own and never rebuilds on its own.
clean:
	rm -f $(KERNEL_BIN) $(BOOTX64) boot/boot.o boot/transition.o
	rm -rf build/libc_obj build/neatvi_obj build/init_launcher
	-$(MAKE) -C src/busybox clean
	rm -f $(BUSYBOX_BIN)
	rm -f $(ROOTFS_IMG) $(ESP_IMG)
