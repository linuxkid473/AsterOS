# Architecture & Key Decisions

## Target
- Darwin 19 (Catalina) xnu-6153.141.1, x86_64 only (Catalina never shipped arm64/Apple Silicon).
- Host: macOS 27 (arm64) with Xcode-beta (clang 21) — cross-compiling x86_64 Mach-O via `-target x86_64-apple-macos10.15`. Apple's ld64/clang are inherently multi-arch, so no separate cross-toolchain is needed for user/kernel Mach-O output.
- Boot target: `qemu-system-x86_64 -machine q35` with OVMF (edk2, from the Homebrew `qemu` package).

## Source dependencies (apple-oss-distributions, pinned tags)
| repo | tag |
|---|---|
| xnu | xnu-6153.141.1 |
| dtrace | dtrace-338.100.1 |
| AvailabilityVersions | AvailabilityVersions-45.11 |
| libplatform | libplatform-220.100.1 |
| libdispatch | libdispatch-1173.100.2 |
| Libsystem | Libsystem-1281.100.1 |

These are needed only to get xnu's own build to compile (headers + ctf tools + libfirehose_kernel). None of them are used to build our userspace.

| repo | tag | subset checked out |
|---|---|---|
| llvm-project | llvmorg-20.1.8 | `llvm`, `cmake`, `compiler-rt` (sparse; grown as later toolchain phases need `clang`/`libcxx`/`libunwind`/`lld`) |

Used for the native-Clang bring-up (see TODO.md Phase 10) — an independent, much larger undertaking from the kernel/userspace pins above. Cross-built from the host's Apple clang 21 targeting `x86_64-apple-macos10.15`, same toolchain pattern as everything else in this project.

## Decision: no dyld / no Libsystem in userspace — superseded, see TODO.md Phase 11
Building real dyld + Libsystem + objc4 + full Libc is a multi-month undertaking on its own (ravynOS spent ~6 months and still lacked working input). XNU's exec path only invokes dyld when a Mach-O has `LC_LOAD_DYLINKER`. A statically-linked Mach-O with no dylib load commands is exec'd directly by the kernel with no userspace runtime dependency at all.
**Decision:** write our own tiny raw-syscall "libc" (BSD syscall ABI: syscall class 2 → `(0x2000000 | number)` in `%rax`, args in `%rdi,%rsi,%rdx,%r10,%r8,%r9`, `syscall` instruction, carry flag set on error) and build our coreutils/shell as static Mach-O against it. This *is* the "tiny userspace" / BusyBox-equivalent phase — documented here as a deliberate deviation from literally porting upstream BusyBox (which targets a real libc/dyld environment we're not building).

**Superseded**: a real, from-scratch dyld (`userland/dyld/`) now exists and is verified live in QEMU — see `TODO.md` Phase 11 for what it does, the ASLR-slide/position-independence gotcha that cost the most time, and its known v1 limitations (no lazy binding exercised yet, no real libSystem, fixed-slot dylib placement). A real libSystem.dylib also now exists (`userland/libSystem/`, wrapping `userland/libc/`) — see `TODO.md` Phase 12. A real libobjc (`userland/libobjc/`, genuine nonfragile-ABI2 metadata layout, real `.m` files compile and run unmodified) also now exists — see `TODO.md` Phase 13 and the libobjc decision section below; still no launchd yet at this point in the timeline (see Phase 14). Static, dyld-free linking remains the default for anything that doesn't need a shared dependency — BusyBox/coreutils were deliberately *not* migrated to dynamic linking when libSystem landed, to avoid touching the already-verified Phase 9 boot path.

## Decision: libobjc ABI-compatibility scope (see TODO.md Phase 13)
The on-disk metadata clang emits for an `.m` file — `class_t`/`class_ro_t`/
`method_t`/`ivar_t`/`category_t`/`protocol_t`/`property_t`, the
`__objc_classlist`/`__objc_catlist`/`__objc_protolist`/`__objc_selrefs`
sections — is real Apple nonfragile-ABI2 layout, ground-truthed field-by-
field against `otool`/`objdump` output on a real compiled probe object, not
approximated. This is the part real `.m` source (compiled unmodified with
the host's own clang) actually depends on, so it's the part that has to be
exactly right.

Everything the runtime keeps for its *own* bookkeeping — nothing external
ever reads these — is not ABI-constrained and was kept deliberately simple:
- **Refcounts**: a global linear side table (object → extra retain count),
  not Apple's isa-embedded inline refcount. Purely an internal performance
  characteristic; no compiled `.m` code can observe which one is in use.
- **Weak references**: a side table (owner → list of weak slot pointers),
  zeroed on `dealloc`, instead of a real weak table with the finer
  ordering guarantees Apple's version provides under real concurrency.
- **Autorelease pools**: one global stack, not per-thread — this project
  has no real threads yet (`pthread_create` is a stub that always fails),
  so per-thread pools would be untestable dead complexity.
- **Method cache**: a small fixed-size per-class open-addressing bucket
  table, not Apple's real `cache_t` bit-packed layout.
- **dyld → libobjc handoff**: a single hardcoded path check
  (`/usr/lib/libobjc.A.dylib`) that dyld resolves and calls `_objc_init`
  on directly, rather than a generic `_dyld_objc_notify_register`-style
  callback-registration API. Correct with exactly one client; would need
  real work to generalize to more.

One real semantic requirement, not a simplification: ARC's
`objc_retainAutoreleasedReturnValue` fast-path reclaim
(`objc_autorelease_try_reclaim_last` in `autorelease.c`) turned out to be
necessary for correctness, not an optional performance trick — ground-
truthed with a double-free bug during Phase 13 verification. Full account
in `TODO.md` Phase 13 and the comments in `arc.c`/`autorelease.c`.

## Decision: root filesystem = MOCKFS + in-memory RAMDisk (no disk driver at all)
Investigated three options:
1. **Real disk (AHCI/NVMe) + HFS+.** Apple's AHCI/NVMe storage kexts are NOT open source; ravynOS had to write their own AHCI driver from scratch (~6 months of effort) to get this far. HFS+ itself is also a separate kext, not statically linkable without patches. Rejected — out of scope for "smallest possible" system, and user explicitly said to skip this (no AHCI).
2. **`imageboot`/`di_root_image` (.dmg root, like the macOS installer).** Requires the closed-source `IOHDIXController` (DiskImages kext). Rejected.
3. **`MOCKFS` (bsd/miscfs/mockfs) + BSD's built-in RAMDisk mechanism.** Fully in xnu source tree already, statically linkable, needs **zero drivers**:
   - `IOKitBSDInit.cpp`'s `IOFindBSDRoot()` looks for a device-tree node `/chosen/memory-map` with a property named `"RAMDisk"` = `{uintptr_t phys_base, uintptr_t size}`. If present, it calls `mdevadd()` automatically, creating `/dev/md0` — a memory-backed block device (`bsd/dev/memdev.c`), no boot-arg needed for this part.
   - boot-arg `rd=md0` selects that device as root (this specific `md`-name recognition in `IOFindBSDRoot` is **not** gated by any config option — always compiled in).
   - `MOCKFS` (`bsd/miscfs/mockfs/`) is a purpose-built read-only 3-node filesystem ("Boot from an executable"): `/`, a `/dev` mountpoint, and one file node that **is** the raw contents of the root memory device, exposed as an executable. It is gated by the `<mockfs>` attribute in `config/MASTER` (off by default) — **patch:** add `mockfs` to `FILESYS_BASE` in `config/MASTER.x86_64`.
   - Net effect: our bootloader loads exactly one file into memory — our statically-linked `init` Mach-O binary — describes it via the device-tree RAMDisk property, and the kernel boots straight into it as PID 1. No filesystem image, no block driver, no HFS.
4. **Writable files (`/tmp`, `/etc`, general `mkdir`/`rm`/`cat` targets):** MOCKFS is read-only and single-file, so real file storage (for `mkdir`, `rm`, `cat`/`echo` redirection) is handled **in-process by our init/shell** using an internal, purely in-memory tree (see Phase 5 docs), not via a second on-disk filesystem. `mount` is still a real syscall — used to mount `devfs` onto `/dev` (the one directory MOCKFS provides for it), which is a genuine kernel-backed mount.

## Boot protocol (x86_64, ground-truthed from pexpert/pexpert/i386/boot.h)
- `boot_args` struct is exactly 4096 bytes (`kBootArgsVersion2`), fields: Revision/Version/efiMode/debugMode/flags, `CommandLine[1024]`, EFI `MemoryMap`/size/descsize/descver, `VideoV1`, `deviceTreeP`/`deviceTreeLength` (physical addr + length of our flattened device tree blob), `kaddr`/`ksize`, EFI runtime service fields, `PhysicalMemorySize`, etc.
- Device tree is Apple's own flat format (`pexpert/pexpert/device_tree.h`): each node = `{uint32 nProperties; uint32 nChildren}` followed by that many `{char name[32]; uint32 length; <value, 4-byte padded>}` property records, then `nChildren` child nodes depth-first. We only need `root -> chosen -> memory-map -> prop "RAMDisk" = {phys_base, size}`.
- **Custom EFI bootloader required**: Apple's real `boot.efi` is not open source. We write a minimal UEFI application (gnu-efi) that: loads `mach_kernel` (Mach-O, parses `LC_SEGMENT_64`), loads our `init` binary into a separate `AllocatePages(EfiLoaderData)` region, builds the device tree blob + `boot_args`, calls `GetMemoryMap`/`ExitBootServices`, and jumps to the kernel entry point.

## Input: PS/2, not USB/AHCI
Per explicit direction: keep input simple — poll the i8042 PS/2 controller (ports 0x60/0x64) directly for keyboard scancodes rather than writing a USB HID or AHCI stack. QEMU's `q35` machine includes a PS/2 controller by default. Output uses the existing xnu serial/video console code already in pexpert.

## Deviations from a literal reading of the phase list
- ~~"BusyBox" is implemented as our own tiny multicall static Mach-O binary~~ —
  superseded (see `TODO.md` Phase 9): real upstream BusyBox 1.36.1 now builds and
  runs as a static, dyld-free Mach-O against `userland/libc/`, a from-scratch libc
  shim built specifically to give BusyBox the POSIX surface it expects (still no
  libSystem/dyld — same raw-syscall philosophy as everything else in this project,
  just a much larger libc surface than the original tiny multicall binary needed).
- Root filesystem is not a general-purpose on-disk filesystem; see MOCKFS decision
  above -- **also superseded**: the root filesystem is now a real FAT16 image
  (`src/xnu/bsd/miscfs/fat16lite`, a from-scratch minimal FAT16 driver), not MOCKFS.
  MOCKFS's single-file-RAMDisk model couldn't support BusyBox's separate applet
  binary, real directories, or file creation (`mkdir`/`rm`/`echo >`), which is why
  the switch happened; the decision writeup above is kept for historical context
  but no longer describes the running system. Full read/write support (mkdir,
  create, write, remove, rmdir) is implemented and verified live in QEMU — see
  `TODO.md` Phase 9, item 10.
