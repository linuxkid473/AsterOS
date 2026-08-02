# Status / TODO

## Phase 1 — Workspace + sources: DONE
xnu-6153.141.1, dtrace-338.100.1, AvailabilityVersions-45.11, libplatform-220.100.1,
libdispatch-1173.100.2, Libsystem-1281.100.1 cloned into `src/`. Local SDK copy at
`build/SDKs/MacOSX10.15.sdk` (cloned from host's MacOSX27.0.sdk, header-patched).

## Phase 2 — Build XNU kernel: DONE, verified with a from-scratch clean rebuild
`make SDKROOT=... ARCH_CONFIGS=X86_64 KERNEL_CONFIGS=DEVELOPMENT` exits 0 with the
BUILD/ dir deleted and rebuilt from nothing — zero make error markers anywhere in
the log. Output copied to `build/kernel/kernel.development`: a 14MB Mach-O 64-bit
x86_64 executable with `_pstart`/`_slave_pstart`/`_vstart` symbols present.

14 patches applied (see `patches/0001`-`0014`), roughly:
- Build environment: SDK/xcrun shim for a modern host Xcode (0001, 0002), an obsolete
  clang flag (0003), ~20 new clang-21 default-warning categories that didn't exist
  when this xnu version was written (0004).
- Scope cuts: NFS (0005), netboot (0012), and the lldbmacros interactive-debugging
  install step (0014, 39 of 60 files are Python-2-only — Apple's own tooling, not
  something we broke) — all things this project has no use for.
- Real upstream bugs fixed, not just modernized: undefined PCI-config identifiers in
  `cpuid.h`/`hpet.c` (0006), a missing comma silently concatenating two malloc-zone
  name strings (0011), a `NULL`-vs-`bool` argument bug in `IOPMrootDomain.cpp` (part
  of 0010).
- Toolchain version-skew (this xnu predates the `iig`/Xcode it's being built with):
  `IORPC::kernelContent` missing field (0009), `OSAction::CreateWithTypeName` missing
  entirely, both declaration and generated-dispatch `_Impl` (0010).
- Python 2→3 ports for two small codegen/syntax-check scripts (0008).
- **The deep one (0013):** the kernel's real x86_64 entry point
  (`osfmk/x86_64/start.s`) loads absolute addresses of high-canonical-but-physically-
  low-loaded symbols truncated to 32 bits — a long-standing, intentional xnu idiom for
  code that runs before paging is enabled. Modern `ld` added a hard validation that
  rejects this outright ("32-bit pointer used in 64-bit code" / "fixup error
  kind=ptr32"), with no linker-flag escape hatch found after trying ~10 candidates.
  Fixed at the source level: replaced every such load (9 sites, including one
  `ljmpl $sel,$off` direct far-jump that has no relative-addressing form at all, fixed
  via an indirect far jump through a runtime-patched pointer) with mechanisms using
  only relocation kinds the linker accepts, verified to produce numerically identical
  runtime values.

## Phase 3 — Boot to console in QEMU: IN PROGRESS, deep into IOKit bootstrap
Custom UEFI bootloader (`boot/`, hand-written against clang+lld-link, no
gnu-efi — see `docs/architecture.md`), boot_args struct per
`pexpert/pexpert/i386/boot.h`, hand-built flattened device tree, QEMU
`-machine q35 -cpu Haswell` with OVMF.

**Verified via serial console (`-serial file:...`), not guessed:** the
kernel now boots past the 64→32 mode transition, xnu's own page-table
bootstrap (`Idle_PTs_init`), `tsc_init`, `pmap_bootstrap`/`vm_page_bootstrap`,
zone/zalloc init, workqueue init, and IOKit's own bootstrap
(`kdp_core`, `IOService`/`IOConfigThread` driver matching) — printing a
correct `Darwin Kernel Version 19.6.0 ...` banner and a correctly-read
`Boot args: -v keepsyms=1 debug=0x144` line. It currently panics at
`IOPlatformExpert.cpp:1994` (`IOPanicPlatform::start`): "Unable to find
driver for this platform" — IOKit's device-tree-driven personality matching
correctly finding no platform-expert kext for our device tree, because real
Mac platform experts (`AppleACPIPlatformExpert` etc.) are proprietary kexts
not present in the open-source xnu tree. This is the expected boundary
between "Mach/BSD kernel boot" (done) and "IOKit hardware driver matching"
(Phase 4/5 territory — needs either a minimal custom platform-expert
personality or a patched fallback instead of `IOPanicPlatform`).

Getting here required diagnosing and fixing five independent, real bugs (in
addition to patch 0015, tracked separately since it's an xnu source change):
1. **QEMU CPU model selection.** `qemu64` fails xnu's own vendor/family
   check (`cpuid_set_cpufamily()`, "Unsupported CPU" panic — it isn't
   `GenuineIntel`/a recognized family). Settled on `-cpu Haswell` (see
   patch 0015 for why, and for the Skylake-Client dead end).
2. **Device tree structure.** Every node — including the root — needs a
   `"name"` property. `nProperties == 0` is not "a node with no properties,"
   it's an explicit end-of-list sentinel `skipProperties()`/`find_entry()`
   check for; a `0`-property root can never be descended into by
   `DTLookupEntry`'s path-walking, so `/chosen` (and thus the
   `"random-seed"` property `PE_get_random_seed`/`bootseed_init_bootloader`
   require, or an early "Expected N seed bytes" panic follows) is
   unreachable no matter how correct the rest of the tree is.
3. **EFI runtime-services memory mapping.** xnu's `efi_init()`
   (`osfmk/i386/AT386/model_dep.c`) maps every `EFI_MEMORY_RUNTIME`
   descriptor at `VirtualStart | VM_MIN_KERNEL_ADDRESS`; since this
   bootloader never calls `SetVirtualAddressMap()` (no use for EFI runtime
   services — no NVRAM/variable access, no reboot-via-firmware), every
   descriptor's `VirtualStart` was `0`, so every runtime region collided at
   the same virtual address (page 0), and whichever OVMF firmware region
   was processed last permanently clobbered the low identity map. Fixed by
   stripping `EFI_MEMORY_RUNTIME` from every descriptor before
   `ExitBootServices` — this kernel has no use for the mechanism anyway.
4. **Where to place boot_args/the device tree/the memory-map buffer.**
   Three placement strategies tried and rejected before landing on the
   right one — see the long comment in `boot/boot.c` right above
   `g_low_alloc_next`'s use for the full blow-by-blow (each rejected
   attempt caught a *different* real bug: an unmapped-access triple fault,
   a `pmap_lowmem_finalize()` unmap-while-still-in-use crash, and a
   `zalloc`-reuse content corruption proven via a direct lldb memory read).
   Final fix: bump-allocate immediately after the kernel's real Mach-O
   segments and report `ba->ksize` as covering the extra data too, so xnu
   treats the whole region as part of its own kernel image — never freed,
   never reused, by construction, with no EFI attribute/type games needed.
5. **tsc_init() divide-by-zero** — see patch 0015.

Every one of these was root-caused by attaching lldb to QEMU's gdbstub
(`-s -S`), not guessed at — several (bugs 3 and 4 in particular) required
walking actual page tables and reading raw physical memory by hand to
confirm the failure mechanism before writing a fix.

## Phase 4 — Root filesystem: IN PROGRESS, blocked on a boot stall

MOCKFS enabled (`mockfs` in `FILESYS_BASE`, `config/MASTER.x86_64`). RAMDisk
loading (`boot/boot.c`) and the `/chosen/memory-map` `"RAMDisk"` device-tree
property are implemented and confirmed loading correctly (`[boot] loaded
launchd RAMDisk ...` prints, `readBooterExtensions()` correctly walks past
the `"name"`/`"RAMDisk"` properties). Userspace (`userland/`) is fully
written and builds (Phase 5 code exists, just unverified end-to-end since
boot doesn't reach it yet).

**Currently blocked**: the kernel hangs (genuine idle, not a crash or busy
loop) immediately after `Registering: IOService:/IOGenericPlatformExpert/chosen`,
inside `IOService::doServiceMatch()`. Five real, independent bugs were found
and fixed while chasing this (a build-time link-address regression, an
unlocked shared PRNG race, two build breaks from properly enabling
`NO_KEXTD`, and an unbounded IOKit synchronous-match wait) — all worth
keeping, none of them turned out to be the actual cause of this specific
hang. Full investigation notes, everything ruled out with evidence, and
recommended next steps: **`patches/0016-boot-thread-stall-at-chosen-registration.md`**.

## Phase 5 — Tiny userspace: NOT STARTED
Raw-syscall libc + static Mach-O coreutils/shell (no dyld/Libsystem). See
`docs/architecture.md` for the BSD syscall ABI notes (class-2 syscall numbers,
`syscall` instruction, carry-flag error convention).

## Phase 6/7 — Init + interactive shell: NOT STARTED
Depends on Phases 3-5.

## Phase 8 — Stabilization: NOT STARTED

## Phase 9 — Real upstream BusyBox as PID 1: DONE, full checklist verified live
Supersedes the "BusyBox → our own tiny multicall binary" deviation below —
this project now runs **real, unmodified BusyBox 1.36.1** (`src/busybox/`,
vendored from upstream, config at `src/busybox/.config`: `ash` + `ls` `cat`
`echo` `mkdir` `rm` `pwd` `uname`, `CONFIG_STATIC=y`,
`CONFIG_FEATURE_SH_STANDALONE=y`, `CONFIG_ASH_INTERNAL_GLOB=y` to dodge
needing `glob.h`). Note: the prior status text above (Phases 1-8) predates
the switch from MOCKFS to a real FAT16 root filesystem (`bsd/miscfs/fat16lite`)
and is stale on that point — FAT16 is what's actually in use now.

**Built:** `userland/libc/` — a from-scratch libc shim (no dyld/Libsystem,
same raw-syscall philosophy as the old `userland/syscall.h`, just far more
complete): headers ground-truthed field-by-field against
`src/xnu/bsd/sys/*.h` (not guessed), a working `malloc` (mmap-backed
first-fit), minimal stdio/dirent/pwd-grp/time, a real signal trampoline
(ground-truthed against `src/libplatform/src/setjmp/x86_64/_sigtramp.s`),
and setjmp/longjmp (own register-save convention, not Apple's
pointer-authenticated one — see `libc/include/setjmp.h`). BusyBox compiles
and links against it as a static, dyld-free Mach-O (`-nostdlib -static -e
_start`) — verified via `otool -l`: no `LC_LOAD_DYLINKER`.

BusyBox's own build system (`scripts/trylink`) hardcodes GNU-ld-only
`-Wl,--start-group`/`--end-group`, which Apple's `ld64` rejects outright;
worked around with a thin `clang` wrapper
(`build/tools/bin/cc-nogroup`) that strips those two tokens, same spirit as
the existing `build/tools/bin/ar` wrapper (BSD `ar` refuses to create an
empty archive the way GNU `ar` silently does, which busybox's Makefile
relies on for subdirs with zero enabled applets). The actual final link is
done by hand (`src/busybox/link_manual.sh`) rather than via `trylink`,
since `trylink` has no notion of our separate libc object set.

**Boots to a live interactive ash prompt** (`boot/esp.img`, `/sbin/init` →
tiny launcher in `userland/init_launcher.c` → `execve("/bin/busybox",
{"busybox","sh",NULL})`). `pwd` confirmed correct (prints `/`). Verified via
serial console over several boot-debug-fix cycles, each a real bug fixed by
reading actual disassembly/kernel debug logs, not guessed:
1. **Duplicate `environ` definition** across 3 `.c` files — ld64 silently
   coalesced them as common symbols, but it was fragile; consolidated to a
   single definition in `libc/src/start.c`.
2. **`tcflag_t` was `unsigned int`, kernel's is `unsigned long`**
   (`src/xnu/bsd/sys/termios.h`) — made `struct termios` the wrong size,
   which is baked into the `TIOCGETA`/`TIOCSETA` ioctl command encoding
   (`_IOR`/`_IOW` embed `sizeof(struct termios)`), breaking `isatty()`.
3. **FAT16 file fragmentation** — repeated `mcopy -o` overwrites onto the
   same image fragmented busybox's later clusters; our `fat16lite` driver's
   `pager_map_to_phys_contiguous` assumes one contiguous cluster run, so
   pages past the fragmentation point mapped to wrong physical memory
   (manifested as a `_applet_name` global-write page fault deep in
   busybox's own `main()`, root-caused via `otool -tv` disassembly + `nm`,
   not guessed). Fix: `mdeltree` + fresh `mmd`/`mcopy` on every deploy
   (see the pattern in the boot-test commands in this session) instead of
   overwriting files in place — a real defragmentation, not a workaround.
4. **`getcwd(NULL, 0)` unsupported** — ash's `pwd` builtin
   (`shell/ash.c:getpwd()`) uses this glibc/BSD auto-allocate extension;
   our `getcwd()` just returned `ERANGE` for `size==0`, so `pwd` printed
   nothing. Fixed in `libc/src/syscalls.c`.
5. **`CONFIG_BUSYBOX_EXEC_PATH` defaulted to `/proc/self/exe`** — no
   `/proc` here, so BusyBox's standalone-shell self-reexec (used for any
   applet not flagged NOFORK/NOEXEC) always failed; set to the real
   `/bin/busybox` in `.config`.
6. **`GETOPT_RESET()` is hardcoded glibc-style** (`optind = 0`,
   `include/libbb.h:1373`, the `#ifdef __GLIBC__` guard is stubbed to
   `#if 1`) — our `getopt()` didn't special-case `optind==0` as "reset",
   so it scanned `argv[0]` (the applet name itself) as the first operand,
   found no `-`, returned -1 immediately without advancing `optind`, and
   callers doing `argv += optind` (e.g. `ls_main`) were left with
   `argv[0]` still `"ls"`, misread as a path to stat. Fixed in
   `libc/src/getopt.c`.
7. **`readdir()` buffer overrun** — was `memcpy(&dirp->cur, raw,
   sizeof(dirp->cur))` unconditionally (~1KB), instead of the record's
   real `d_reclen`, reading past the actual on-wire dirent into
   uninitialized buffer content and never guaranteeing `d_name` was
   NUL-terminated. Fixed in `libc/src/dirent.c`.
8. **Missing carry-flag check on every raw syscall wrapper** — the real
   xnu class-2 syscall ABI signals error via the carry flag with the
   *positive* errno left in `%rax` (never negated), but
   `libc/src/syscall_raw.h`'s `raw_syscallN` helpers only ever read
   `%rax` as the return value, so *every* syscall error (`open()` ENOTSUP,
   `getdirentries64()` EBADF, etc.) was silently misread as a small
   "successful" return value instead of failing loudly. This was the
   central diagnostic breakthrough that unblocked everything below — e.g.
   `sys_getdirentries64` "returning 0 bytes" from item 7's investigation
   was actually `open()` failing with errno 45 (ENOTSUP), misread as
   fd=45. Fixed by capturing `setc` after every `syscall` instruction in
   `raw_syscall0`-`raw_syscall6` and `fork()`'s own inline asm
   (`libc/src/syscall_raw.h`, `libc/src/syscalls.c`), and removing a
   duplicate, equally-buggy `raw3()` helper in `libc/src/signal.c`.
9. **`fat16lite_open`/`close`/`access` wired to the generic "always fail"
   `err_open`/`err_close`/`err_access` stubs** — meant a real userspace
   `open(2)` could never succeed against any fat16lite vnode at all (the
   exec()/pager path that loads and runs the launcher and busybox itself
   never goes through `VNOP_OPEN`, so this went unnoticed until real
   `open()` calls — `ls`, `cat`, etc. — were exercised). Implemented real
   vnops in `fat16lite_vnops.c`.
10. **mkdir/create/write/remove/rmdir vnops implemented from scratch**
    (`fat16lite_vnops.c`: `fat16lite_mkdir`, `fat16lite_create`,
    `fat16lite_write`, `fat16lite_remove`, `fat16lite_rmdir`, plus
    `fat16lite_alloc_cluster`/`fat16lite_set_fat_entry`/
    `fat16lite_write_dirent`/`fat16lite_find_free_slot` helpers), needed
    since the driver was read-only-only up to this point. Along the way:
    - `vfs_rootmountalloc_internal()` (`vfs_subr.c`) hardcoded
      `MNT_RDONLY` into every root mount's flags at allocation time,
      overriding the vfsconf's own flags; changed to just `MNT_ROOTFS`.
    - `fat16lite_lookup()` had a hardcoded leftover
      `if (cnp->cn_nameiop != LOOKUP) return EROFS;`, unconditionally
      rejecting any CREATE/DELETE/RENAME-context lookup — a relic from
      when the driver really was read-only. Removed it and implemented
      the standard VFS lookup protocol instead: a not-found lookup on the
      last pathname component in a CREATE/RENAME context must return
      `EJUSTRETURN` (not `ENOENT`), per `vfs_lookup.c`'s `lookup()`/
      `relookup()`.
    - `fat16lite_open()` still unconditionally rejected `FWRITE` (a
      leftover from the same read-only-only era) even after write vnops
      existed — this is what caused `mkdir` to work but
      `echo hi > file` to keep failing with EROFS after the lookup fix:
      `mkdir(2)` never calls `VNOP_OPEN`, but `open(O_CREAT|O_WRONLY)`
      does (`vn_open_auth` opens the vnode `vn_create` just created,
      since fat16lite has no compound-open support), and that open hit
      the stale `FWRITE` check. Fixed by dropping the check.

**Fully verified end-to-end**, live in QEMU, one boot session: `pwd`,
`ls`, `cd`, `mkdir`, `echo > file` (create + write), `cat` (read back
correct content), `echo` (no redirect), `uname -a`, `rm`, and a final `ls`
confirming the removed file is gone. All debug `printf`s added during
this investigation (`fat16lite_vnops.c`, `fat16lite_fsnode.c`,
`vfs_syscalls.c`'s `getdirentries64`/`getdirentries_common`,
`vfs_subr.c`'s `vnode_authorize`/`vnode_attr_authorize`, and the
exec-path tracing in `mach_loader.c`/`kern_exec.c`) have been removed;
re-verified clean after cleanup with a fresh rebuild and boot test.

## Phase 10 — Native Clang/LLVM toolchain: IN PROGRESS, step 1 of many

Goal: `clang hello.c -o hello && ./hello` running natively on AsterOS itself
(the resulting compiler executes on-target, not just cross-compiles for it —
building it necessarily happens by cross-compiling from the macOS host, same
as the kernel and BusyBox were). Per-blocker incremental build strategy:
compiler-rt → libc compat → libunwind → LLVM support libs → Clang frontend →
lld, one link/run-verified step at a time, matching the Phase 1-9 discipline
above.

**Source**: `src/llvm-project` — sparse (`llvm`, `cmake`, `compiler-rt`
only; `clang`/`libcxx`/`libunwind`/`lld` added as each phase needs them),
shallow (`--depth 1`), pinned at tag `llvmorg-20.1.8`. Its own nested git
repo, same pattern as `src/xnu`/`src/dtrace` (gitlink in the parent tree,
no `.gitmodules` needed).

**Step 1 — compiler-rt builtins: DONE.** First link-time blocker for any
future userspace C/C++ program built with our cross clang: soft
arithmetic helpers (`__divti3`, `__udivdi3`, `__multi3`, 128-bit
int/float ops, etc.) that clang emits calls to for operations the x86_64
ISA has no direct instruction for. Without `libclang_rt.osx.a` on the
link line, any such reference is an undefined symbol at link time —
this is the actual first blocker, ahead of anything AsterOS-specific,
since it applies even to a trivial `hello.c`.

Built via compiler-rt's own CMake as a **standalone** project (not through
the top-level `llvm/CMakeLists.txt` runtimes-bootstrap path, which
requires a `clang` target/build in the same invocation we don't have yet —
first real blocker hit and routed around this step): `cmake -S
src/llvm-project/compiler-rt -B build/compiler-rt-build`, host Apple clang
21 as `CMAKE_C_COMPILER`/`CMAKE_CXX_COMPILER` with `-target
x86_64-apple-macos10.15`, `CMAKE_SYSROOT` = the existing local
`build/SDKs/MacOSX10.15.sdk`, `LLVM_MAIN_SRC_DIR`/`LLVM_CMAKE_DIR` pointed
at the sparse `llvm/` checkout so it doesn't need a built `llvm-config`,
`COMPILER_RT_ENABLE_IOS=OFF` (defaults to on given the host's iOS SDKs;
not needed here — cut to keep the build scoped to macOS), sanitizers/XRay/
libFuzzer/profile/memprof/ORC all off (not needed for a bare-arithmetic
builtins library and each pulls in its own dependency chain).

Builds clean (`ninja lib/darwin/libclang_rt.osx.a`, 1123/1123, zero
errors — only benign libtool "no symbols" warnings for translation units
that compile to nothing on x86_64, e.g. `divmodti4.c`). Verified, not
assumed: `lipo -info` on the output fat archive shows an `x86_64` slice;
thinned and `nm`'d, it contains real, correctly-named Mach-O symbols
(`___divti3`, `___udivdi3`, `___multi3`, leading-underscore C convention).
Installed to `build/compiler-rt-install/lib/darwin/libclang_rt.osx.a` —
the exact `<resource-dir>/lib/darwin/libclang_rt.<platform>.a` path shape
the clang driver looks for, ready for the LLVM support libraries / Clang
frontend steps to link against later.

**Next blocker**: nothing to build against it yet — this library is
inert until something actually calls it. The next step is libc
compatibility for what LLVM's own support libraries
(`llvm/lib/Support`) need beyond what `userland/libc/` already has (real
`nanosleep`/nanosecond time, and — the big open question — whether to
add a minimal `pthread` shim now, since `LLVM_ENABLE_THREADS=OFF` avoids
it for the *build itself* but the standalone-runtimes path taken above
means the *next* CMake configure, for `llvm/lib/Support` proper, is the
first one that will actually try to compile and link real LLVM C++
source against `userland/libc/`'s headers — expect the first real
libc-gap errors there, not before.

## Phase 11 — dyld (dynamic linker): DONE, verified live in QEMU
Real dyld, from scratch (`userland/dyld/`) — not the deferred-forever stub the
Phase 1 roadmap and `docs/architecture.md`'s "no dyld" decision originally
called for. Loads LC_LOAD_DYLIB dependencies from disk, rebases (REBASE_OPCODE_*),
binds (BIND_OPCODE_*, both eager and a real lazy `dyld_stub_binder` path), and
resolves symbols via the export trie (LC_DYLD_INFO_ONLY), all interpreted
against the vendored `mach-o/loader.h`. Kernel side needed zero changes —
`mach_loader.c`'s `load_dylinker()`/LC_LOAD_DYLINKER handling was already fully
present and correct, ground-truthed by reading it before writing a line of
dyld code (see the file-by-file breakdown that motivated this phase).

**Verified live**: `build/dyld_obj/dyntest`, a normal libc-based executable
with `LC_LOAD_DYLINKER=/usr/lib/dyld` and `LC_LOAD_DYLIB=/usr/lib/libtest.dylib`,
boots, dyld loads `libtest.dylib` (and its own `/usr/lib/libSystem.B.dylib`
placeholder dependency, see below), calls a function and reads a `const char *`
exported from the dylib correctly, printing `DYNTEST PASS` — confirmed across
multiple runs with different kernel-chosen ASLR slides in one boot session.

**The one real bug, and why it mattered**: mach_loader.c *always* computes and
applies an ASLR slide to the dylinker specifically, regardless of the MH_PIE
bit or a `-no_pie` link (first-hand ground-truth, not something the architecture
doc predicted). dyld must therefore be genuine position-independent code — every
internal reference RIP-relative, correct at any load address with zero fixups of
its own — not just "linked at a fixed non-PIE address," which was the original
(wrong) plan. Getting this fully right needed **both** `-fPIC` (forces
RIP-relative codegen instead of absolute addresses) **and** `-fvisibility=hidden`
plus explicit `__attribute__((visibility("hidden")))` on every cross-TU `extern`
(`g_images`, `_mh_dylinker_header`, `_dyld_stub_binder_entry`) — without the
latter, clang still routes non-hidden externs through a GOT slot that only a
rebase pass would populate, and dyld can never rebase itself (it's the one
providing rebase to everyone else). Confirmed fixed by checking dyld's own
`LC_DYLD_INFO_ONLY` is emitted with `rebase_size 0` — dyld needs *zero* fixups,
same as real dyld.

**Known limitations / deliberate v1 simplifications**:
- Lazy binding (`stub_binder.c`/`stub_binder_asm.S`) is real and ground-truthed
  against ld64's actual `stub_x86_64.hpp` codegen, but *unexercised*: the host's
  modern ld64 segfaults (`IndirectSymbolTableBuilderImpl`, an internal
  `_sideInfo` assertion in `Atom.h`) building lazy stubs without a real
  libSystem providing `dyld_stub_binder`, so `userland/dyld/test/build.sh`
  links with `-bind_at_load` (eager binds only) to sidestep it. Fixing this
  needs either a linker that doesn't crash here (maybe the native ld64 once
  Phase 10 lands) or a from-scratch stub-generation workaround.
- No real libSystem *at this point in the timeline* — superseded, see Phase 12
  below. The host's ld64 hard-refuses to build *any* dynamic executable/dylib
  without linking something named libSystem (checked empirically, applies even
  to an otherwise-empty dylib); at the time this phase landed that was worked
  around with a hand-authored, link-time-only `libSystem_stub.tbd` (zero real
  symbols) plus a real empty placeholder Mach-O shipped at
  `/usr/lib/libSystem.B.dylib` that dyld loaded like any other dependency and
  never actually needed anything from.
- Dylib placement is fixed 256MB slots (`g_next_dylib_base`), not a real VM
  allocator — fine for a handful of dylibs, not a general-purpose scheme.
- No `@rpath`/`@loader_path`, no two-level-namespace subtlety beyond ordinal
  bind, no weak symbols/re-exports, no `mprotect` re-protection after fixups
  (segments stay RWX — no mprotect syscall wired up yet).

## Phase 12 — libSystem.dylib: DONE, verified live in QEMU

A real `MH_DYLIB` at `/usr/lib/libSystem.B.dylib` (`userland/libSystem/`),
replacing the empty placeholder Phase 11 shipped there — every `userland/libc/`
source file compiled `-fPIC` and linked `-dynamiclib`, exporting 568 symbols
(`nm -gU`). Unlike dyld itself, libSystem is loaded *by* dyld like any other
dependency and gets properly rebased/bound at load time, so none of dyld's own
`-fvisibility=hidden`/`DYLD_HIDDEN` self-reference discipline applies here —
ordinary default-visibility PIC is correct and sufficient.

**The real design problem, and how it was solved:** `__libc_start` (the
function that calls this process's `main`) can't live inside the dylib —
`main` only exists once the final executable is linked, so a dylib containing
a direct call to it fails to link with an undefined `_main` (caught
empirically, not anticipated). Split `userland/libc/src/start.c`: environ
storage, `atexit`/`__cxa_atexit`/`__cxa_finalize`, `exit`/`_Exit`/`abort` stay
in `start.c` (now compiled into the dylib — several other libc/src files
reference these directly, e.g. `assert.c`'s `abort()`, so they need to live in
the same image as their callers). `__libc_start` itself, plus
`run_mod_init_funcs()` (needed alongside it: ld64's `section$start$`/
`section$end$__DATA$__mod_init_func` symbols are scoped to *whichever image is
being linked*, so this must stay statically linked per-executable to see that
executable's own mod-init section, not the dylib's), moved to a new
`userland/libc/src/libc_start.c` — matches Darwin's real crt1.o/libSystem
split (crt1.o stays tiny and executable-specific; everything else moves to the
shared library). `crt0.S` (`_start` itself) and `libc_start.o` are the only
two objects still statically linked per-executable.

Also removed `dl_stub.c`'s `_dyld_find_unwind_sections()`: zero callers
anywhere in the tree, and — same class of bug as the mod-init-func issue — it
referenced `_mh_execute_header`/`section$start$__TEXT$__eh_frame` etc., which
are only meaningful for the specific calling image, not a shared library's own
(the comment above it had explicitly documented the "no dyld, exactly one
statically-linked image" assumption this build finally broke). Genuinely dead
code once that assumption stopped holding; deleted rather than reworked.

Added `reboot(int howto)` to `syscalls.c` (`SYS_reboot` was already
`#define`d, just never wrapped) — needed by launchd's shutdown path, Phase 14.

**The self-link quirk:** building libSystem.B.dylib hits the *same* "ld64
refuses an empty dependency list" check as everything else in this project,
but can't be satisfied by dyld's existing `libSystem_stub.tbd` (its
install-name literally *is* `/usr/lib/libSystem.B.dylib` — the dylib being
built here — which ld64 separately refuses as a self-link). Solved with a
second stub/placeholder pair, `userland/libSystem/selflink_stub.tbd` +
a real tiny empty `libSystem_selflink_stub.dylib` shipped alongside the real
thing. Not a genuine runtime circular dependency despite appearances: our
dyld's `find_loaded()` registers an image's path in `g_images[]` before
recursing into its own dependencies (`macho_load.c`), so the nominal
dep-on-the-dep-on-itself resolves to an already-cached (currently-loading)
image with no actual recursion.

**Verified live in QEMU**, not assumed: `userland/libSystem/test/systest`, a
normal dynamically-linked executable (crt0.o + libc_start.o statically linked,
everything else resolved against the real dylib) — `SYSTEST PASS` after real
`printf`, `malloc`/`free`, and a `fork()`/`waitpid()` round trip all running
through actual dyld rebase/bind/export-trie resolution. Re-ran `/bin/dyntest`
(Phase 11's own regression test) immediately after — still `DYNTEST PASS`,
confirming the real libSystem now sitting at `/usr/lib/libSystem.B.dylib`
didn't disturb dyld's existing dependency loading. Also re-ran the full
Phase 9 static-BusyBox checklist (`mkdir`/`cd`/`echo >`/`cat`/`ls`/`rm`) since
the `start.c`/`libc_start.c` split touched code shared with the static build
path — unaffected, `cat` reads back `hello_world` correctly.

Not wired into the top-level `Makefile` — same as dyld itself (also absent
from the `.PHONY` list), built directly via `bash userland/libSystem/build.sh`
and `bash userland/libSystem/test/build.sh`. `userland/mkrootfs.sh` picks up
the real dylib opportunistically if present in `build/`, falling back to the
Phase 11 placeholder otherwise.

**Known v1 limitations:** BusyBox/coreutils stay static (not migrated to link
against libSystem — deliberate, see the plan behind this phase; the Phase 9
verified boot path isn't worth the risk for this pass). `dlopen`/`dlsym`
(`dl_stub.c`) still honestly return `ENOSYS` — untouched, out of scope here.

## Phase 13 — libobjc.A.dylib: DONE, verified live in QEMU

A real Objective-C runtime (`userland/libobjc/`), `/usr/lib/libobjc.A.dylib`,
depending on Phase 12's libSystem. Targets the genuine nonfragile-ABI2
on-disk metadata layout (`class_t`/`class_ro_t`/`method_t`/`ivar_t`/
`category_t`/`protocol_t`/`property_t`) as the host's own clang emits it for
`-target x86_64-apple-macos10.15 -fobjc-runtime=macosx` — every struct in
`objc_abi.h` was ground-truthed field-by-field against `otool`/`objdump`
output on a real compiled probe `.o`, not written from memory. The
regression test (`userland/libobjc/test/test.m`) is unmodified real `.m`
source built with the host's off-the-shelf clang, exactly like a real
Darwin `.m` file would be.

**dyld got two small, additive extensions**, neither touching rebase/bind/
export logic: `image_run_mod_init_funcs()` (`macho_load.c`/`image.h`) walks
any loaded image's `LC_SEGMENT_64` sections at runtime for
`S_MOD_INIT_FUNC_POINTERS` and runs them — previously only the main
executable's own compile-time mod-init section ever ran, dylibs loaded at
runtime never had theirs run at all. And a hardcoded post-bind, pre-init
hook in `dyld_main.c`: if `/usr/lib/libobjc.A.dylib` is among the loaded
images, resolve its exported `__objc_init` via the existing
`image_resolve_export` and call it with every loaded image's mach header,
before any mod-init-funcs run anywhere — this project's intentionally
simplified stand-in for real dyld's generic `_dyld_objc_notify_register`
callback-registration API (one hardcoded client instead of a registration
mechanism, since libobjc is the only client that needs it). Mod-init-funcs
run in reverse image-registration order so dependencies always initialize
before dependents.

**Internal (non-ABI-visible) design**: class realization uses a tagged
pointer in `class_t.data` (`class_rw_t*` OR 1) to distinguish "still points
at the compiler's read-only `class_ro_t`" from "already realized" — the
low bit is otherwise always 0 on a real pointer since the struct requires
alignment, and nothing outside this runtime ever reads `class_t.data`
directly. Realization recurses into the superclass first (required for
correct ivar-offset patching, see below) and is genuinely two-pass at the
image level: pass 1 registers every class in every loaded image, pass 2
(`objc_attach_categories`) attaches every pending category's methods/
protocols/properties — allows a category compiled into one image to
extend a class defined in another, matching real dyld/objc ordering.
Method dispatch (`msgSend.S`, hand-written x86_64) saves all six integer
and eight `xmm` argument registers to a scratch stack area, resolves the
IMP via a small per-class open-addressing cache (`dispatch.c`, our own
bucket format — internal, not ABI-visible) or a full superclass-chain
walk on a miss, then restores registers and tail-jumps into the IMP so
the callee's own `ret` returns directly to the original caller.
`objc_msgSendSuper2` was ground-truthed to dereference
`current_class->superclass` (not just `current_class`) — the actual
difference from the legacy, no-longer-emitted `objc_msgSendSuper`.

**Three real bugs found and fixed during QEMU verification:**
- **Class realization was completely broken** until the tagged-pointer fix
  above landed: `class_rw()` originally read `class_t.data` unconditionally,
  but `data` is never NULL (it's `class_ro_t*` before realization,
  `class_rw_t*` after), so the "already realized" guard was true on every
  class's very first call — `objc_getClass` returned NULL for every class
  despite the raw compiled metadata being perfectly valid. Classrefs still
  resolved fine via ordinary dyld rebase/bind (unrelated mechanism), which
  is why method dispatch partially appeared to work even with zero classes
  actually registered — a misleading symptom that cost real debugging time.
- **Ivar offset patching is not optional.** Real nonfragile ABI2 requires
  the runtime to patch each ivar's `*offset` at realization time using the
  superclass's actual instance size, since the compiler can't know a
  cross-image superclass's real size at compile time. Missing this crashed
  on the first property access through an inherited ivar.
- **ARC's `objc_retainAutoreleasedReturnValue` fast path turned out to be
  semantically required, not a performance optimization** — ground-truthed
  the hard way with a double-free/under-release bug (full account in
  `arc.c`'s and `autorelease.c`'s comments). An autoreleased value
  immediately "claimed" by the caller needs its pending pool release
  canceled (`objc_autorelease_try_reclaim_last`, approximated here as
  "is the top of the single global autorelease stack literally this
  object," correct for the call-adjacent pattern real `-fobjc-arc`
  codegen actually produces) **while still** recording a real extra unit
  of ownership in the side-table refcount — skipping either half
  double-frees or under-retains. Separately: calling `objc_autorelease`
  directly from ARC-compiled code, even from within the same translation
  unit, gets extra retain/release bracketing inserted by the compiler
  around the call regardless of whether the result is used — the test's
  actual `-autorelease` send had to move into its own non-ARC translation
  unit (`test/mrc_helper.m`) to get deterministic, correct behavior.

**Verified live in QEMU**: `build/libobjc_obj/objctest`, real `.m` source
(subclass with an ivar + synthesized property, a category adding protocol
conformance, `[super init]`, ARC-managed alloc/scope-exit release, and an
explicit `@autoreleasepool` block) — `OBJCTEST PASS`. Immediately re-ran
`/bin/dyntest` (Phase 11) and `/bin/systest` (Phase 12) — still
`DYNTEST PASS`/`SYSTEST PASS` — plus the full Phase 9 static-BusyBox
checklist, confirming no regression anywhere in the chain this phase built
on top of.

**Known v1 limitations (documented, not oversights):**
- Refcounts are a global linear side table (object ptr → extra count), not
  Apple's isa-embedded inline refcount — an internal, ABI-invisible
  optimization that no compiled `.m` code or external caller can observe,
  so it was deprioritized. Same story for weak references (side table,
  owner → slot list) instead of a real weak table with zeroing tied into
  deallocation ordering guarantees beyond "on `dealloc`, walk and null."
- A single global autorelease pool stack, not per-thread — this project
  has no real threads yet (`pthread_stub.c` unconditionally fails
  `pthread_create`), so a thread-local stack would be dead complexity
  with nothing to exercise it.
- The `_objc_init` dyld hook is one hardcoded path match on
  `/usr/lib/libobjc.A.dylib`, not a generic callback-registration API —
  fine with exactly one client, would need real work to support more.
- No `@synchronized`, no exceptions (`@try`/`@catch`/`@throw`), no
  associated objects, no `NSObject` beyond what `Root.m` implements by
  hand (alloc/init/dealloc/retain/release/autorelease/class/
  isKindOfClass:/respondsToSelector:/isEqual:/hash/description/
  conformsToProtocol:) — enough for the test surface, not a full
  Foundation-scale root class.
- Fixed-size static tables throughout (`OBJC_MAX_SELECTORS`,
  `OBJC_MAX_CLASSES`, `MAX_REFCOUNTED`, `MAX_AUTORELEASED`, ...), not
  dynamic growth — correct and simple for this project's current scale,
  documented as a real ceiling rather than silently wrapping.

## Known deviations from a literal reading of the task (documented, not oversights)
- ~~BusyBox → our own tiny multicall static binary~~ — superseded, see Phase 9 above.
- ~~Root filesystem → MOCKFS + RAMDisk~~ — superseded: the actual root filesystem is
  now a real FAT16 image (`bsd/miscfs/fat16lite`, `boot/fat16.img`), not MOCKFS; the
  Phase 3/4 narrative above predates that switch and is stale on this point.
- Input → PS/2 polling (per explicit direction), not USB/AHCI.
- NFS client/server and netboot compiled out of the kernel entirely (out of scope,
  and their code fails dozens of new-clang warnings-as-errors / doesn't link without
  NFS — see patches/0005, 0012).
- lldb kernel-debugging macros (tools/lldbmacros) skipped entirely — Python 2-only,
  no interactive-debugging use case here — see patches/0014.
