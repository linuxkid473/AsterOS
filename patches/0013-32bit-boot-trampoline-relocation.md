# Patch: fix 32-bit boot trampoline addressing for modern `ld`

**Component:** xnu (`osfmk/x86_64/start.s`)

**This is the deepest patch in the set** — a real incompatibility between a
long-standing xnu idiom and a newer, stricter validation in the linker
bundled with modern Xcode, not just a warning-level nuisance. It blocked the
final kernel link (everything else had already compiled).

## The error

```
ld: 32-bit pointer used in 64-bit code in '_pstart'+0x185 (osfmk/.../start.o)
```

## Root cause

`osfmk/x86_64/start.s` is the kernel's real entry point (`pstart`/`_start`,
`slave_pstart` for AP CPUs, `hibernate_machine_entrypoint`, and
`acpi_wake_prot` for S3 resume) — code that runs **before paging is
enabled**, while the CPU is still in 32-bit protected mode, so it can only
use legacy 32-bit-encoded instructions (no `REX` prefixes, no `%rip`-relative
addressing, which requires long mode).

To get the address of a symbol like `low_eintstack` into `%esp` in this
environment, the code does:

```asm
movl	$EXT(low_eintstack), %esp
```

`low_eintstack` lives in the `__HIB` section, linked at a high canonical
kernel address (`KERNEL_HIB_SECTION_BASE` in `makedefs/MakeInc.def`, e.g.
`0xffffff8000100000`). `movl $symbol, %reg` encodes the symbol's address as
a 32-bit immediate — i.e. it truncates the address to its low 32 bits. This
is **intentional**: the bootloader physically loads `__HIB` such that its
physical load address's low 32 bits equal the low 32 bits of its high
canonical virtual link address (confirmed independently by
`osfmk/x86_64/start.s`'s `SWITCH_TO_64BIT_MODE` macro loading `%cr3` — which
always holds a *physical* address — the exact same way via `BootPML4`). So
the truncated value is exactly the correct, usable 32-bit physical/flat
address for this pre-paging code.

Older `ld` accepted this without complaint. The linker bundled with the
Xcode this project cross-builds with added a hard validation that a 32-bit
absolute relocation must "round-trip" (zero-extending the truncated 32-bit
value must reproduce the original 64-bit target address) — which no
high-canonical-address symbol can ever satisfy, intentional truncation or
not. Confirmed by an isolated repro (`movl $symbol, %reg` where `symbol`
lives at a `0xffffff80...`-range address, linked standalone with matching
`-pie -image_base ... -pagezero_size 0x0` flags) reproducing the identical
`ld: 32-bit pointer used in 64-bit code` message. Tried and ruled out before
concluding there's no linker-flag escape hatch: `-read_only_relocs
suppress` (gets past a *different*, earlier "Illegal text-relocations"
check, but not this one), `-no_pie` (produces a related but distinct
`fixup error (kind=ptr32)... 32-bit pointer overflow` — same root issue,
not gated on PIE), `-ld_classic` (no longer supported by this `ld`, ignored
with a warning), `-w`, and half a dozen other plausible flag names — none
exist for this specific check in this `ld`.

## Fix

Replaced every `movl $EXT(symbol)[+const], %reg` absolute-truncating load in
this file (9 sites total: `BootPML4` ×2, `BootPDPT`, `protected_mode_gdtr`,
`master_gdtr`, `mp_slave_stack`, `low_eintstack` ×3, across the BSP entry,
AP entry, hibernate-resume entry, and ACPI-wake entry paths) with a
`LOAD_LOW32(reg, expr)` macro using the classic 32-bit position-independent-code
"get current PC" technique:

```asm
#define LOAD_LOW32(reg, expr) \
	call	1f		;\
1:	pop	reg		;\
	addl	$((expr) - 1b), reg
```

`call 1f` / `pop reg` puts the **runtime** low-32-bit program counter into
`reg` (a real, current value — not a link-time constant). `(expr) - 1b` is a
compile-time-computable *displacement* between two symbols in the same
linked image; unlike an absolute address, a displacement between two nearby
high-canonical addresses is small and has no round-trip ambiguity, so the
linker accepts it as an ordinary signed 32-bit relocation. Adding the two
produces **the exact same numeric value** the original code obtained via
truncation — this is a mechanical, semantics-preserving rewrite, not a
behavior change: because this trampoline runs at a physical address whose
low 32 bits match its link address's low 32 bits (the same premise the
original code relied on), "get my current low32 PC" plus "compile-time
offset to target" equals "target's low32 address", identically to direct
truncation.

Requires a valid, writable stack at the call site (the `call` pushes a
return address). Verified true at all 9 sites: two (`low_eintstack` in
`pstart`, `mp_slave_stack` in `slave_pstart`) are literally the first
instructions establishing this code's own stack, relying on the stack
already handed off from whatever transferred control here (the bootloader
for the BSP path, an AP-startup trampoline for the slave path) — standard
practice for any x86 boot protocol, and also relied upon implicitly by the
original code (which needed *some* valid `%esp` to eventually run C code
against). The other 7 sites run after one of those two, so the stack is
already this code's own by then.

This same numeric identity is why it's safe to leave `-Wl,-pie` in place
(restored after using its removal only as a diagnostic experiment) — the
fix isn't PIE-conditional; it addresses the relocation type itself.
