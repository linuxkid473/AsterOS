# Patch: undefined `cfgAdr`/`cfgDat`/`XeonCapID5`/`lpcCfg`

**Component:** xnu (`osfmk/i386/cpuid.h`, `osfmk/i386/hpet.c`)

**Problem:** `osfmk/i386/cpuid.h`'s `is_xeon_sp()` and `osfmk/i386/hpet.c`'s `map_rcbaArea()` reference `cfgAdr`, `cfgDat`, `XeonCapID5` (cpuid.h) and `lpcCfg` (hpet.c) that are never `#define`d anywhere in this xnu-6153.141.1 snapshot — a real bug in the public source drop, not a clang-version issue (`error: use of undeclared identifier`).

**Analysis:** Both functions are narrow, real-hardware chipset-quirk workarounds (multi-socket Xeon Scalable Platform capability detection, and Intel PCH RCBA mapping) that are dead code on our QEMU q35 target regardless of their exact values — this hardware doesn't exist in emulation.

**Fix:** Defined the missing identifiers using the architectural/documented values:
- `cfgAdr = 0xCF8`, `cfgDat = 0xCFC` — the standard, universal x86 PCI CONFIG_ADDRESS/CONFIG_DATA I/O ports (architectural, not chipset-specific).
- `XeonCapID5 = 0x8001F398` — the CONFIG_ADDRESS encoding for PCI config space 1:30:3:0x98, per the existing comment in the code (`enable=1, bus=1, dev=30, func=3, reg=0x98`).
- `lpcCfg = 0x8000F800` — the CONFIG_ADDRESS encoding for PCI 0:31:0, where the LPC bridge always lives on Intel chipsets (matches the same constant used by e.g. coreboot/SeaBIOS for this purpose).

Since neither function is reachable in any code path that matters on our target, correctness of these constants isn't load-bearing for us — they're defined for architectural correctness (so the code means what it says) rather than because we depend on the exact behavior.
