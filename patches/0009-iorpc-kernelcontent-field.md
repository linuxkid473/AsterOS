# Patch: add missing `kernelContent` field to `struct IORPC`

**Component:** xnu (`iokit/DriverKit/IORPC.h` and `iokit/IOKit/IORPC.h` — xnu carries two copies of this same struct)

**Problem:** `iig` (the DriverKit IDL compiler bundled with the modern Xcode we're
cross-building with) generates code like:

```cpp
IORPC rpc = { .message = ..., .reply = ..., .sendSize = ..., .replySize = ...,
              .kernelContent = &buf.msg.content.__hdr };
...
IORPCMessage * msg = rpc.kernelContent;
```

for every `.iig` file in `iokit/DriverKit/` (`OSObject.iig`, `OSAction.iig`,
`IOService.iig`, `IOUserClient.iig`, `IODataQueueDispatchSource.iig`, ...) —
dozens of call sites across the whole DriverKit IPC surface. But
xnu-6153.141.1's own `struct IORPC` only has `message`/`reply`/`sendSize`/
`replySize` — no `kernelContent`. This is a real toolchain/source version-skew
bug: this `iig` targets a newer DriverKit RPC ABI (with a fast-path pointer to
the message content held alongside the Mach message pointers) than the IORPC.h
this xnu version ships.

**Analysis:** We don't use the DriverKit userspace RPC runtime anywhere in this
project (no DriverKit dexts), so the exact runtime semantics of this field are
irrelevant to us — we only need the generated code to compile.

**Fix:** Added `IORPCMessage * kernelContent;` to the end of `struct
IORPC` in both copies of the header. Purely additive (new field at the end),
so it doesn't disturb the existing four fields any other code depends on.
(First attempt used `const IORPCMessage *`, but the generated code assigns
`rpc.kernelContent` to a plain non-const `IORPCMessage *` in several places —
dropped the `const` to match actual usage.)

Took two attempts to find both copies: `iokit/DriverKit/IORPC.h` fixed first
(the one under the same directory as the `.iig` sources), but the actual
plain-kernel-build compile of the generated `.iig.cpp` files turned out to
resolve `#include <IORPC.h>`-equivalent lookups to `iokit/IOKit/IORPC.h`
instead — a byte-for-byte duplicate struct definition at a different path —
confirmed by diffing `BUILD/obj/EXPORT_HDRS/iokit/{DriverKit,IOKit}/IORPC.h`
after a clean rebuild, only the `IOKit/` copy was missing the fix.
