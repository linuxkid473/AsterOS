# Investigation: boot thread stalls registering "/chosen" (Phase 4, in progress)

## Status

Kernel boots cleanly through Mach/BSD/VM/zone bootstrap and IOKit startup,
successfully registers the platform expert root nub and `IOPMrootDomain`, then
permanently idles (confirmed via `ps`: ~0.1-0.3% CPU sustained over
multiple 60-280s windows with zero further serial output) right after:

```
Registering: IOService:/IOGenericPlatformExpert/chosen
```

This is a **real, reproducible hang inside `IOService::doServiceMatch()`**
while matching the `/chosen` device-tree node — not a crash, not a busy loop
(confirmed genuinely idle via `machine_idle`/`processor_idle`/`idle_thread`
backtraces from repeated lldb/gdbstub attaches), and **not caused by our
custom `RAMDisk`/`memory-map` device-tree additions** (reproduced identically
with `memory-map` and its child entirely removed from the device tree — see
"Ruled out" below).

## Fixes applied this session (real bugs, kept regardless of the chosen-hang outcome)

1. **`SLIDE=0x10` build-time regression** (`build-kernel.sh`). The kernel's
   static link address (`KERNEL_TEXT_BASE` in
   `xnu/makedefs/MakeInc.def`) depends on a `SLIDE` make variable that
   defaults to 0 if unset, giving a load address of `0xffffff8000200000`
   (phys `0x200000`) — a physical address OVMF's own EFI memory map has
   already claimed, so our bootloader's `AllocatePages(AllocateAddress, ...)`
   for the kernel's `__TEXT` segment fails outright
   ("`AllocatePages(segment) failed`"), well before ExitBootServices. `SLIDE=0x10`
   moves the load address up to `0xffffff8002200000` (phys `~34MiB`), which is
   free. This was previously fixed by passing `SLIDE=0x10` ad hoc on the `make`
   command line in an earlier session, which was lost once that shell session
   ended (nothing durable recorded it). Now baked into `build-kernel.sh` itself.

2. **Unlocked shared PRNG state race** (`osfmk/prng/prng_random.c`). Since this
   project has zero kexts, `prng_ready` never becomes 1, so `read_erandom()`'s
   early-boot-only, deliberately-unlocked fallback path (a single shared
   `erandom` HMAC-DRBG state) stays in permanent use for the kernel's entire
   lifetime — including after interrupts/preemption/multiple threads are live,
   which upstream never expects. Added a simple atomic
   test-and-set spinlock (`erandom_lock`, using `__sync_lock_test_and_set`/
   `__sync_lock_release` rather than `lck_spin_t`, since `early_random()` is
   called from `i386_init.c` before lock groups are guaranteed to exist)
   guarding both `early_random()`'s lazy DRBG init and `read_erandom()`'s
   generate call. This is a genuine correctness fix (confirmed via lldb: two
   real, different call sites — `IOPMrootDomain::start()`'s boot/wake UUID and
   `OSKext::removeKextBootstrap()`'s KLD-segment scrub — both hit this shared
   state) but **did not by itself fix the chosen-registration hang** (see
   "Ruled out" below) — keep it regardless, it's still a real bug.

3. **`NO_KEXTD` enabled for x86_64** (`config/MASTER.x86_64`, added to
   `IOKIT_BASE`). This project has zero kexts and no kextd userspace daemon;
   without `NO_KEXTD`, `IOService::probeCandidates()`'s kextd-check-in logic
   and related `#if !NO_KEXTD` code paths stay compiled in but can never be
   satisfied. Enabling it surfaced two latent, unrelated x86_64 build bugs
   that had to be fixed to make it compile at all:
   - `libsa/bootstrap.cpp`: a `#if NO_KEXTD` block (freeing prelinked-kext
     data for kexts skipped on non-developer/non-ramdisk boots) had only ever
     been implemented for arm/arm64; the x86_64 `#else` branch was a bare
     `#error`. Since we have zero prelinked kexts, this code is unreachable at
     runtime for us regardless — left as a documented no-op.
   - `libkern/os/log.c`: `#define FIREHOSE_USES_SHARED_CACHE NO_KEXTD` couples
     the firehose logging shared-cache path directly to `NO_KEXTD`, but that
     path's `segLOWESTTEXT` symbol is only ever defined for arm/arm64
     (`osfmk/arm{,64}/arm_vm_init.c`) — enabling `NO_KEXTD` on x86_64 caused a
     link failure (`_segLOWESTTEXT` undefined). Forced
     `FIREHOSE_USES_SHARED_CACHE` to `0` unconditionally on `__x86_64__`,
     independent of `NO_KEXTD`.
   - **Enabling `NO_KEXTD` did not fix the chosen-registration hang either**
     (reproduced identically afterward) — kept anyway since it's still the
     architecturally-correct setting for a kext-less kernel, and removes a
     class of future kextd-wait bugs.

4. **Bounded synchronous-match wait** (`iokit/Kernel/IOService.cpp`,
   `IOService::startMatching()`). The generic synchronous-matching retry loop
   (`do { doServiceMatch(); ...; assert_wait(...); thread_block(); } while
   (waitAgain)`) waited on `assert_wait(..., THREAD_UNINT)` with **no
   timeout at all** — if `doServiceMatch()` ever increases a service's busy
   count without anything later decrementing it (module-stall,
   `kIOServiceModuleStallState`, or any other busy-count producer that
   assumes an async worker/kextd will eventually finish), the calling thread
   blocks forever. Changed to `assert_wait_timeout(..., 100, NSEC_PER_MSEC)`
   with a 20-retry cap (2s worst case per service), logging and giving up
   instead of hanging. This is defensible, generically-safe hardening for a
   kext-less system — **but it also did not fix the chosen-registration
   hang**, meaning whatever's actually stuck is not this particular wait
   (either `doServiceMatch()` for `/chosen` doesn't reach this retry loop's
   blocking branch at all, or the real block is inside `doServiceMatch()`
   itself, before it would ever return to this loop).

## Ruled out (with direct evidence, not guesses)

All confirmed via lldb attached to QEMU's gdbstub (`-s`, no `-S`) using a
deliberate `gBS->Stall()` in `boot/boot.c` right before `ExitBootServices` as
an attach window (kernel segments are already copied into physical memory by
that point, so breakpoints set then survive into the running kernel) — see
the extensive breakpoint-based tracing method below for future sessions.

- **`read_random()`/`read_erandom()` hanging**: disproven. A breakpoint on
  `read_random()`'s own return address (computed from `objdump` of its
  caller) fires promptly; single-stepping ~60 instructions into
  `ccdrbg_generate()`'s internals showed clean, bounded, forward-only
  progress (no repeated addresses). The earlier appearance of a hang here was
  a **methodology artifact**: attaching lldb takes a variable, uncontrolled
  amount of real time, so different test runs sampled different, unrelated
  points along boot's linear (non-looping) progression — comparing breakpoint
  hits *across separate qemu launches* is not valid evidence of a hang by
  itself. Only sustained low-CPU idle measured *within a single undisturbed
  run*, or a breakpoint's return address never firing despite 4+ minutes of
  patient `continue`-ing *within one attach*, count as real evidence.
- **`OSKext::removeKextBootstrap()`'s `read_frandom()` call on the KLD
  segment (0x3000 bytes)**: disproven. Replaced with a `memset(0)` (kept as
  a diagnostic-only change, currently reverted — see `libkern/c++/OSKext.cpp`
  git history if re-testing) and rebuilt; identical hang at the identical
  log line.
- **Our custom `RAMDisk` device-tree property / `memory-map` child node**:
  disproven. Temporarily removed `/chosen`'s `memory-map` child entirely
  (`boot/boot.c`, `chosen`'s `nChildren` 1→0, `memory-map` node emission
  `#if 0`'d out) and rebuilt; identical hang at the identical log line, just
  with `"Can't read booter memory map."` instead of the two `"X" not a kext`
  lines (expected, since `/chosen/memory-map` no longer exists for
  `readBooterExtensions()` to find — confirms the change took effect).
- **`IOPMrootDomain::start()` itself**: disproven. Its own `read_random()`
  call for a boot/wake UUID (16 bytes) was traced via breakpoint on its own
  return address and confirmed to return promptly; the function's registry
  path (`IOService:/IOGenericPlatformExpert/IOPMrootDomain`) registers
  successfully and is not revisited.
- **`NO_KEXTD`'s kextd-wait mechanism specifically**: disproven (see fix #3
  above — enabling it changed nothing).
- **The one unbounded `assert_wait` in `IOService::startMatching()`'s sync
  retry loop specifically**: disproven (see fix #4 above — bounding it
  changed nothing).

## Where things stand: narrowed to `doServiceMatch()` itself

Breakpointed `IOService::doServiceMatch()`'s entry (`rdi` = `this`) across the
whole boot sequence: it's called for the root nub, then (after
`probeCandidates()` matches and instantiates `IOGenericPlatformExpert`) for
that instance, then for **two more distinct objects** in quick succession —
almost certainly `IOPMrootDomain` and `/chosen`, matching the log's 3
`"Registering:"` lines (root nub's own registration doesn't count as a 4th
`doServiceMatch` caller — `IOGenericPlatformExpert`'s post-probe registration
is the "4th" object here). `probeCandidates()` only fires for the very first
(root) call — expected, since neither of our two personalities
(`IOGenericPlatformExpert`, `IOPanicPlatform`) target anything but
`IOPlatformExpertDevice`, so `/chosen` legitimately matches nothing and
`doServiceMatch()`'s `matches->getCount() == 0` branch skips
`probeCandidates()` entirely (see `IOService.cpp` ~line 4703).

Breakpointing `IOLockLock`/`lockForArbitration`/`unlockForArbitration` after
the 4th `doServiceMatch()` call shows **substantial, genuine, varied lock
activity across dozens of distinct objects** (65 confirmed `continue`-and-hit
cycles across a 280-second window, all resolving normally, touching many
different `rdi` addresses) before the *next* one — the 66th — never returns.
This does **not** look like a tight infinite loop (CPU stays near-idle
throughout, and a real spin loop recomputing `findDrivers()`/locks
repeatedly would show high, not low, CPU) — it looks like real matching/
notification-cascade work that is legitimately extensive, terminating in one
specific, not-yet-identified blocking wait.

## Recommended next steps (for whoever continues this)

1. Get a **symbolicated backtrace at the exact 66th-iteration stopping
   point**. The catch: `target create <kernel binary>` before `gdb-remote`
   gives working symbols/backtraces (via `bt`) but appears to break software
   breakpoint insertion against this specific QEMU gdbstub setup (breakpoints
   report success but never trap) — confirmed by a clean A/B test (identical
   `machine_idle` breakpoint: fires in 3s with a bare `gdb-remote` connect,
   never fires in 20s+ with `target create` done first). Bare connect +
   raw `-a <address>` breakpoints (addresses from a fresh `nm
   build/kernel/kernel.development | awk '{print $1,$3}' | sort` — **must be
   regenerated after every kernel rebuild**, addresses shift) is the only
   combination confirmed to reliably trap. Getting *both* (working
   breakpoints *and* a real `bt`) in the same session is the missing piece —
   try attaching bare, letting it run to the 66th-iteration stall point, then
   *without issuing any more breakpoint commands*, doing `target create`
   only for symbolication and `bt`/`frame variable` (not for anything that
   requires inserting new code).
2. Manually walking the kernel's global `threads` doubly-linked list
   (`osfmk/kern/processor.c`'s `queue_head_t threads`, `threads_count`; each
   `thread_t`'s intrusive link is the `threads` member,
   `offsetof(struct thread, threads)` was 760 on the pre-session-fixes build —
   confirmed via `expression -- (unsigned long)&(((struct thread *)0)->threads)`,
   recompute after any rebuild) to find the actual blocked thread's saved
   `continuation`/`wait_event` (not `idle_thread`'s, which is a different
   thread and unhelpful) is conceptually the right move, but hit an LLDB
   expression-evaluator limitation: chaining more than one level of pointer
   indirection through `$`-prefixed persistent variables intermittently
   fails with `Interpreter couldn't read from memory` or
   `Couldn't dematerialize a result variable`. Worth revisiting with smaller,
   more incremental expressions, or by reading the chain through raw
   `memory read` commands instead of the C expression evaluator.
3. Consider instrumenting `IOService.cpp` directly (temporary `IOLog`s
   bracketing suspect sections within `doServiceMatch()`/notification
   invocation for the specific object) rather than continuing pure
   live-debugging — the io=0x7f / io=0xffffffff `gIOKitDebug` boot-args were
   tried and produced no additional output around the stall, meaning
   whatever's blocking isn't behind an existing `gIOKitDebug`-gated log
   statement.
