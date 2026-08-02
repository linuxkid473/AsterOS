# Patch: install `os/firehose_buffer_private.h` etc. for the kernel's own os_log

**Component:** SDK header install (`build/SDKs/MacOSX10.15.sdk/usr/include/os/`)

**Problem:** `libkern/os/log.c` (xnu's own in-kernel os_log implementation, not the
`libfirehose_kernel` static library built separately in patches 0001/0002) does
`#include <os/firehose_buffer_private.h>`, which isn't part of xnu, libplatform,
or the base SDK — it's a libdispatch header (`libdispatch/os/firehose_buffer_private.h`)
that in a real Apple build gets installed into the SDK by a libdispatch install
step we never ran (we only built the `libfirehose_kernel` static-library target).

**Fix:** Copied directly into the SDK, same pattern as the other missing private
headers in this project. Note the destination: xnu's own kernel-side compiles use
`-nostdinc` and only add `-I$(SDKROOT)/usr/local/include/kernel` (`INCFLAGS_SDK` in
`makedefs/MakeInc.def`) — not the normal `usr/include` tree — so these need to live
under `usr/local/include/kernel/os/`, matching exactly where libdispatch's own
`libfirehose_kernel.xcconfig` would have installed them (`PRIVATE_HEADERS_FOLDER_PATH
= /usr/local/include/kernel/os`) had we run its `install` step instead of just `build`:
- `libdispatch/os/firehose_buffer_private.h` → `usr/local/include/kernel/os/`
- `libdispatch/os/firehose_server_private.h` → `usr/local/include/kernel/os/` (referenced alongside it)
- `libplatform/private/os/base_private.h` → `usr/local/include/kernel/os/` (a transitive include of `firehose_buffer_private.h`)

(Also copied to `usr/include/os/` for anything compiled outside the `-nostdinc` kernel path, e.g. `libfirehose_kernel` itself.)

**Follow-up (`OS_FIREHOSE_SPI`):** Even once found, `firehose_buffer_private.h`'s
entire body (including the `#ifdef KERNEL` branch declaring
`__firehose_buffer_tracepoint_reserve`, `__firehose_buffer_tracepoint_flush`,
`FIREHOSE_BUFFER_KERNEL_CHUNK_COUNT`, etc.) is wrapped in `#if OS_FIREHOSE_SPI`.
In a real Apple build this macro doesn't need to be passed at compile time —
libdispatch's own header-install step runs the copied headers through
`unifdef -DOS_FIREHOSE_SPI=1` (see `COPY_HEADERS_UNIFDEF_FLAGS` in
`libfirehose_kernel.xcconfig`), permanently baking the condition to true in the
installed copy. We skipped that install step (raw-copied the header instead),
so the raw `#if OS_FIREHOSE_SPI` is left unresolved and defaults to false,
silently compiling to an empty header. Fixed by adding `-DOS_FIREHOSE_SPI=1` to
`CFLAGS_GEN`/`CXXFLAGS_GEN` instead of running `unifdef` ourselves — same
effective result, one line instead of a new build step.
