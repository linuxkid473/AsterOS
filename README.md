# AsterOS

A from-source bring-up of a minimal Darwin 19 (Catalina, xnu-6153.141.1) x86_64
system: real XNU kernel + BSD/Mach, a custom UEFI bootloader (Apple's own
`boot.efi` isn't open source), a tiny raw-syscall userspace (no dyld/Libsystem),
booting to an interactive shell in QEMU.

See `docs/architecture.md` for the full set of design decisions and why they
were made (root filesystem strategy, boot protocol, why no dyld, etc).
See `patches/` for every source patch applied to the upstream Apple sources,
each with a rationale. See `logs/` for build/boot logs. See `TODO.md` for
current status and what's left.

## Layout

- `src/` — pinned checkouts of xnu, dtrace, AvailabilityVersions, libplatform,
  libdispatch, Libsystem (apple-oss-distributions, exact tags in
  `docs/architecture.md`).
- `build/` — all build output. `build/SDKs/MacOSX10.15.sdk` is a local,
  header-patched copy of the host's SDK (no real Catalina SDK exists on this
  host); `build/tools/bin` holds host build tools including an `xcrun` shim
  (see patch 0002) and the ctf tools.
- `boot/` — the custom EFI bootloader source.
- `rootfs/` — the tiny userspace source (raw-syscall libc, coreutils, init, sh).
- `patches/` — one markdown file per patch, numbered, each self-contained.
- `docs/` — architecture notes.
- `logs/` — build and boot logs.

## Building the kernel

```
./build-kernel.sh
```

Builds ctf tools, installs AvailabilityVersions/libplatform headers, builds
`libfirehose_kernel.a`, then does `exporthdrs` followed by the kernel proper
into `build/kernel/kernel.development`. Verified (Phase 2 complete): `make
... KERNEL_CONFIGS=DEVELOPMENT` exits 0 from a fully-deleted `BUILD/` dir,
producing a 14MB Mach-O 64-bit x86_64 executable with `_pstart`/`_vstart`
present. 14 patches were needed to get there against this modern host's
toolchain — see `patches/0001`-`0014.md`, and `patches/0013` in particular
for the deepest one (the kernel's own 32-bit boot trampoline vs. a newer
linker validation).

## Booting

```
qemu-system-x86_64 -machine q35 -m 2048 \
  -bios <ovmf firmware from Homebrew qemu, see boot/> \
  -drive format=raw,file=<ESP image with our bootloader + kernel> \
  -serial stdio
```

See `boot/` for the bootloader and ESP image build steps once Phase 3 lands.
