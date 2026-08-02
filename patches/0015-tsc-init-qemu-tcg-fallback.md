# Patch: fall back to a 1:1 TSC/bus ratio when QEMU/TCG doesn't populate the calibration MSRs

**Component:** xnu (`osfmk/i386/tsc.c`)

## The crash

Divide-by-zero (`#DE`, trap 0) inside `tsc_init()`, on real hardware never hit
because real firmware/CPUs always leave real data in the MSRs this code
reads.

## Root cause

`tsc_init()`'s `default:` case (every Intel family except Kaby Lake/Skylake,
which use CPUID leaf 0x15 instead, and Penryn, which uses `IA32_PERF_STS`)
reads `MSR_FLEX_RATIO` and `MSR_PLATFORM_INFO` to derive `tscGranularity`
(the bus-to-TSC tick ratio):

```c
msr_platform_info = rdmsr64(MSR_PLATFORM_INFO);
flex_ratio_max = (uint32_t)bitfield(msr_platform_info, 15, 8);
tscGranularity = flex_ratio_max;
```

Under QEMU with TCG (software emulation — no `-enable-kvm`, the only option
on this project's arm64 host), these MSRs aren't populated with real bus-ratio
data; `rdmsr64(MSR_PLATFORM_INFO)` reads back `0`, so `flex_ratio_max` and
therefore `tscGranularity` are `0`. Later:

```c
tscFCvtt2n = busFCvtt2n / tscGranularity;   /* divide by zero */
```

This is a genuine QEMU/TCG CPU-emulation gap (these MSRs are typically only
populated when running under KVM with hardware acceleration, or on real
silicon), not a bug in xnu or in this project's bootloader — confirmed via
lldb attached to QEMU's gdbstub, single-stepping through `tsc_init`'s
`default:` case and reading back `rbx`/`r14` (N/M) and the MSR values
directly after the `rdmsr` instructions.

## Fix

Treat an unreported ratio as 1:1 (TSC ticks == bus ticks) instead of
dividing by it — the same convention `tsc_init()` itself already uses a few
lines later when `tscFreq == busFreq`:

```c
if (tscGranularity == 0) {
    tscGranularity = 1;
}
```

This only changes behavior when the MSR data was already unusable (0); real
hardware reporting a real ratio is unaffected.

## Related, not fixed here: CPU model choice

This patch alone doesn't make every QEMU CPU model boot. `-cpu qemu64`
fails an earlier, unrelated check (`cpuid_set_cpufamily()` doesn't recognize
its vendor string as `GenuineIntel`/a known family, panicking with
"Unsupported CPU" — see `osfmk/i386/cpuid.c`). `-cpu Skylake-Client` avoids
`tsc_init`'s `default:` case entirely (it uses the Kaby Lake/Skylake branch,
reading CPUID leaf 0x15 for the TSC/crystal-clock ratio instead of MSRs) but
QEMU/TCG's leaf 0x15 emulation returns numerator=0, denominator=0, tripping
`assert(N != 0)` — and because that assertion fires so early in boot (well
before console/panic infrastructure that a *later* assertion failure can
rely on is fully up), the resulting `Assert()` → `panic()` path itself
crashes with an unrelated-looking "jumped to a null pointer" `#PF` rather
than a clean assertion message. `-cpu Haswell` plus this patch was the
combination that actually got through `tsc_init()` cleanly — documented here
so a future change of QEMU CPU model doesn't silently reopen either failure
mode.
