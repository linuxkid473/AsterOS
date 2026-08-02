# Real-hardware bring-up runbook: HP Pavilion 500-201a

The xHCI + USB HID keyboard driver (`osfmk/usb/{pci,xhci,usb_hid}.c`, see
`docs/architecture.md`'s "USB stack" section and `TODO.md` Phase 15) is fully
verified in QEMU but has **not** been run on real hardware — this session had
no physical access to the HP Pavilion 500-201a. This is what to do to test it
there and get useful information back for the next round of fixes.

## 1. Build and write the image

```
make image
```

produces `boot/esp.img` — a FAT-formatted ESP containing `EFI/BOOT/BOOTX64.EFI`
(the bootloader), `mach_kernel`, and the root filesystem. Write it directly to
a USB flash drive (this **erases the drive** — pick the right `diskN`, check
with `diskutil list` first):

```
diskutil unmountDisk /dev/diskN
sudo dd if=boot/esp.img of=/dev/rdiskN bs=1m
```

Boot the Pavilion from that USB drive (F9/Esc at power-on for the boot menu on
most HP desktops, or set it first in UEFI boot order). **Disable Secure Boot**
in the UEFI setup menu first — this bootloader is unsigned.

If the firmware refuses to boot a raw FAT16 image from a USB stick as
removable media (varies by firmware; most UEFI implementations support this
for FAT32 but FAT16 support is less consistent), the fallback is to partition
the USB drive normally (GPT, one FAT32 ESP partition) and copy `EFI/`,
`mach_kernel`, and the rootfs image across instead of `dd`-ing the raw image —
ask if this comes up and it isn't already handled.

## 2. Capture what the kernel prints

Boot messages (`kprintf`, gated on the `-v` boot arg already baked into this
project's command line) are what carries every `[pci]`/`[xhci]`/`[hid]` log
line this driver produces. Two ways to capture them, in order of preference:

- **Serial port**, if the Pavilion has one (header or physical DB9) and you
  have a USB-serial adapter: connect it, open a terminal at 115200 8N1 (e.g.
  `screen /dev/tty.usbserial-XXXX 115200` on macOS, or any serial terminal
  app), and capture the whole session to a text file from power-on. This is
  the same technique this project has used throughout QEMU bring-up
  (`-serial file:...`, see `TODO.md`'s Phase 3 notes) and gives a clean,
  greppable log.
- **No serial available**: verbose boot output also goes to the physical
  display (the `-v` flag affects both). Screen-record (phone camera is fine)
  from power-on through several seconds after you plug in a USB keyboard, and
  send that — even a legible photo of the screen at the point it stops
  (if it stops) is useful.

## 3. What to actually do once it's booted (or stuck)

1. Let it boot all the way. If it reaches the BusyBox `#` shell prompt, that's
   already a good sign — it means the boot path up to `kernel_bootstrap_thread`
   completed and USB init at least didn't hang anything.
2. If a USB keyboard wasn't already plugged in, plug one into any port and
   watch for `[xhci] port N: connect detected` / `... reset complete` / `[hid]
   slot N: keyboard ready` lines (or their absence).
3. Type something. Confirm it lands in the shell.
4. Try unplugging and replugging the keyboard once, same as the QEMU test.
5. If PS/2 is also present on this machine (unlikely on a 2013-era HP
   Pavilion desktop, but check), confirm it still works too — the two drivers
   are independent and shouldn't interfere, but worth a quick check.

## 4. If something goes wrong

Send back whatever log/recording you captured, plus:
- Where it stopped (last `[pci]`/`[xhci]`/`[hid]` line, or a description of
  what's on screen if it never got that far).
- Whether it's a hang (nothing moving) vs. a panic (usually an all-caps panic
  string) vs. it just silently not detecting the keyboard.

The most likely real-hardware-specific failure points, ranked by likelihood,
so you know what a given symptom probably means even before I look at logs:

- **USB Legacy Support handoff never completing** (the "BIOS did not release
  ownership within 5s" warning in the log, not a hang — the driver proceeds
  anyway) — some real BIOS/firmware implementations have known bugs here;
  this being logged rather than silently hanging is the point of that
  bounded-timeout design.
- **A port range assumption being wrong** — the driver reads the Supported
  Protocol Capability rather than assuming port numbers, but if the Pavilion's
  xHCI controller reports something unusual there, ports might not be
  classified as expected. The `[xhci] Supported Protocol: ...` log lines show
  exactly what was read.
- **A BAR or MMIO mapping issue** specific to this chipset — would show up as
  a failure right after the `[xhci] ...: BAR0 phys=... mapped at ...` line, or
  a reset timeout immediately after.
- **A genuinely different xHCI controller quirk** not exercised by QEMU's
  software model at all (QEMU's `qemu-xhci` is a reasonably faithful spec
  implementation, but isn't the same silicon).

Given the debug logging, we should be able to tell which of these it is (or
rule all of them out) from the captured log alone.
