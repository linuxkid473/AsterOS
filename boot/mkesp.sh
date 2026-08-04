#!/bin/bash
# Assembles boot/esp.img, the FAT16 ESP that OVMF loads BOOTX64.EFI from.
# Reformatted from scratch every time for the same reason as
# userland/mkrootfs.sh (avoid in-place-overwrite fragmentation).
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."

ESP_IMG="boot/esp.img"
ESP_SIZE_MB=300
KERNEL_BIN="build/kernel/kernel.development"

rm -f "$ESP_IMG"
dd if=/dev/zero of="$ESP_IMG" bs=1m count="$ESP_SIZE_MB" status=none
mformat -i "$ESP_IMG" -c 16 -r 32 -h 16 -n 63 -v ESP ::

mmd -i "$ESP_IMG" ::/EFI
mmd -i "$ESP_IMG" ::/EFI/BOOT

mcopy -i "$ESP_IMG" boot/BOOTX64.EFI ::/EFI/BOOT/BOOTX64.EFI
mcopy -i "$ESP_IMG" "$KERNEL_BIN" ::/mach_kernel
mcopy -i "$ESP_IMG" boot/fat16.img ::/fat16.img

# Quiet-boot splash (see boot/gen_splash.py) -- optional: boot.c falls back
# to the plain text console if this isn't present.
if [ -f boot/splash.raw ]; then
	mcopy -i "$ESP_IMG" boot/splash.raw ::/splash.raw
fi

echo "ESP assembled: $ESP_IMG"
mdir -i "$ESP_IMG" ::
