# Patch: disable new clang-21 `-Werror` warning categories

**Component:** xnu (`makedefs/MakeInc.def`, `CFLAGS_GEN`)

**Problem:** xnu builds with `-Werror` throughout, which is fine against the clang it was written for (~2019), but clang 21 enables several new warnings by default that fire thousands of times across xnu's own (correct, intentional) code:
- `-Wreserved-identifier`: xnu's own BSD/kernel headers legitimately use `__foo`/`_Foo`-style reserved names throughout (`__size`, `__prev`, `_Dispatch`, `_RESERVEDIODMACommand9`, ...) — this is xnu's own reserved namespace, not a bug.
- `-Wcast-function-type-strict`: IOKit's `IOWorkLoop::Action` callback pattern intentionally casts through a generic function-pointer signature; this is the established IOKit callback idiom, not a bug.
- `-Wfour-char-constants`: xnu uses classic four-char-code constants (`'APPL'`-style) deliberately in several places.
- `-Wunused-but-set-variable` / `-Wformat`: mostly genuine (harmless) staleness from a 6-years-newer compiler having stricter format/type checking than what this xnu version was written against; downgraded from hard errors to warnings rather than fully silenced, so real problems still show up in the build log without blocking the build.

**Fix:** Added a block of `-Wno-*` flags to both `CFLAGS_GEN` and `CXXFLAGS_GEN` (C++ needed its own copy — `CXXFLAGS_GEN` doesn't inherit from `CFLAGS_GEN`). This is a blanket flag change rather than thousands of per-file patches, consistent with "prefer minimal patches" — the alternative (rewriting every reserved identifier, every `_Atomic` use, etc. in xnu's own headers) would be enormous churn against code that isn't actually wrong. Final list, built up over several rebuild iterations as each new category surfaced:

- `-Wno-reserved-identifier` — xnu's own `__foo`/`_Foo` reserved-namespace names (legitimate, it's the kernel's own namespace)
- `-Wno-cast-function-type-strict` / `-Wno-cast-function-type-mismatch` — IOKit/BSD's function-pointer-cast callback idioms (`IOWorkLoop::Action`, `cdevsw`/`tty` op tables, etc.)
- `-Wno-four-char-constants` — deliberate four-char-code constants
- `-Wno-pre-c11-compat` — clang 21 flags `_Atomic`/`_Static_assert` (276 + 56 occurrences) as "incompatible with C standards before C11" even though the effective compile mode is gnu17/gnu++1z (C11-and-later); reproduced in isolation and confirmed this fires independent of `-std`, i.e. it's not actually about the active standard
- `-Wno-declaration-after-statement` — xnu is full of C89-style-adjacent code that declares mid-block; valid since C99, which xnu has always targeted
- `-Wno-suggest-override` / `-Wno-suggest-destructor-override` — libkern C++ (`OSObject` subclasses) predates C++11 `override`
- `-Wno-switch-default` — plenty of intentionally-exhaustive `switch` statements with no `default`
- `-Wno-ms-bitfield-padding` — an MSVC-ABI-compatibility diagnostic irrelevant to a Mach-O/Itanium-ABI kernel
- `-Wno-ossharedptr-misuse` — libkern's `OSPtr`/`OSSharedPtr` smart-pointer helper pattern
- `-Wno-error=unused-but-set-variable`, `-Wno-error=format`, `-Wno-error=implicit-fallthrough` — downgraded to warnings rather than fully silenced, so real problems still show up in the build log without blocking the build
- `-Wno-anon-enum-enum-conversion` — `pexpert/i386/pe_serial.c` does arithmetic between two distinct anonymous enums (a common, harmless C idiom)
- `-Wno-missing-designated-field-initializers` — struct literals that only set some fields, relying on C's implicit zero-fill for the rest (standard, intentional)
- `-Wno-misleading-indentation` — a handful of `bsd/dev/dtrace/fasttrap.c` sites clang's new heuristic flags but which are correct
- `-Wno-void-pointer-to-int-cast` — DTrace's `bsd/dev/dtrace/systrace.c` intentionally packs a small syscall number into a `void *` callback argument and unpacks it later; safe by construction, not a truncation bug

- `-Wno-unaligned-access` — `bsd/net/necp.h` and `osfmk/kdp/kdp_core.h` have packed structs with intentionally-unaligned union members (wire/core-dump formats); the padding is deliberate
- `-Wno-c99-designator` — array designated initializers (`[X] = val`), which is completely standard, valid C since 1999; clang's default mode just started flagging it as "an extension" more aggressively
- `-Wno-unnecessary-virtual-specifier` — `IOPerfControl.h` marks overrides `virtual` inside a `final` class; harmless redundancy, not a bug
- `-Wno-gnu-folding-constant` — a few sites fold a VLA to a constant-size array (a long-standing, harmless GNU extension)
- `-Wno-error=unused-variable` — downgraded rather than silenced (distinct diagnostic from `-Wunused-but-set-variable` above)
- `-Wno-null-pointer-subtraction` — 157 occurrences of pointer-subtraction idioms (largely `offsetof`-style patterns) clang now considers technically-UB; correct in context throughout
- `-Wno-array-parameter` — a `char cdhash[static 20]` parameter-bound mismatch (a still-correct, still-common C idiom for "at least N elements")
- `-Wno-unterminated-string-initialization` — `osfmk/console/serial_console.c`'s `nmi_string[32] = "afDIGHr84A84jh19Kphgp428DNPdnapq"` is a deliberate 32-byte non-NUL-terminated magic/passphrase buffer (compared bytewise, never treated as a C string) — exactly the `nonstring`-attribute pattern clang is suggesting, just not yet annotated as such in this xnu version
- `-Wno-pointer-to-int-cast` — a `caddr_t`→`uint32_t` cast (same class as the earlier `-Wno-void-pointer-to-int-cast`, just clang's narrower diagnostic for the non-`void*` case)
- `-Wno-c23-compat` — xnu uses `true`/`false` as ordinary identifiers in a few spots; harmless until this codebase is ever compiled as actual C23 (it isn't)
- `-Wno-deprecated-non-prototype` — a handful of old K&R-style function definitions without prototypes
- `-Wno-nrvo` — a new "missed named-return-value-optimization" advisory warning; a compiler performance hint, not a correctness issue
- `-Wno-implicit-int-float-conversion` — a `uint64_t`→`double` narrowing inside `panic()`'s formatting path; acceptable precision loss for a debug/panic message
- `-Wno-single-bit-bitfield-constant-conversion` — `type field:1` bit-fields assigned literal `1` throughout xnu as boolean-style flags; technically stores as -1 in a signed 1-bit field, but every use is a truthiness check (`if (field)`), never an equality compare, so the representation doesn't matter
- `-Wno-tautological-value-range-compare` — `bsd/netinet6/esp_output.c` (IPsec ESP, not part of our boot path at all) compares a 4-bit field against 96; whether or not that's a latent logic bug in Apple's own ESP code, we don't use IPsec anywhere in this project
- `-Wno-sign-compare` — one isolated `for (int i = 0; i < CS_CDHASH_LEN; ++i)` in `bsd/kern/kern_exec.c` where `CS_CDHASH_LEN` resolves unsigned; the only sign-compare hit across the entire build (this diagnostic doesn't otherwise fire, so this is a narrow, low-risk addition, not a broad new hole)
- `-Wno-tautological-constant-compare` — 28 occurrences of `1 << width`-style bit-mask macros (e.g. `bsd/kern/kern_lockf.c`) used directly as a boolean, always true by construction and intentionally so
- `-Wno-missing-noreturn` — clang suggesting `kern_malloc.c`'s `kmeminit()` could be marked `noreturn` (it panics on every exit path); a style suggestion, not a bug
- `-Wno-global-constructors` — `libsa/lastkernelconstructor.c` deliberately declares a `__attribute__((constructor))` function (its entire purpose, per the file name, is kernel-link-order bookkeeping); clang now warns on this attribute unconditionally in C++/ObjC++ mode

xnu already builds with `-Weverything` and a long list of curated `-Wno-*` exclusions (`makedefs/MakeInc.def`'s `WARNFLAGS_STD`) — these additions follow that same established pattern for categories clang 21 added since xnu-6153 was written, rather than introducing a new mechanism.
