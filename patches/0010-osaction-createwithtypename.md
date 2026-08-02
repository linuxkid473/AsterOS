# Patch: add missing `OSAction::CreateWithTypeName`

**Component:** xnu (`iokit/DriverKit/OSAction.iig`, `iokit/Kernel/IOUserServer.cpp`)

**Problem:** Same class of bug as patch 0009 (`IORPC::kernelContent`): the
modern `iig` we're cross-building with generates, for every per-method
DriverKit completion-action subclass (e.g. `IOUserClient.iig`'s
`KernelCompletion`), a call to:

```cpp
OSAction_IOUserClient_KernelCompletion::CreateWithTypeName(
    this, targetmsgid, msgid, referenceSize, typeName, action);
```

but xnu-6153.141.1's `OSAction` (`iokit/DriverKit/OSAction.iig`) only declares
the older `Create(target, targetmsgid, msgid, referenceSize, action)` (no
`typeName`) — `error: no member named 'CreateWithTypeName'`. The real modern
`OSAction::Create`-family allocator uses `typeName` to instantiate the actual
requested subclass via its `OSMetaClass`; this xnu's `OSAction::Create`
(`iokit/Kernel/IOUserServer.cpp`) always allocates a plain base `OSAction`
(`OSTypeAlloc(OSAction)`) — a real, not just cosmetic, version gap.

**Decision:** We don't use the DriverKit userspace RPC/completion runtime
anywhere in this project, so subclass-correct allocation isn't something we
depend on — we only need this to compile and not misbehave if ever exercised.

**Fix:**
- Declared `CreateWithTypeName(OSObject *, uint64_t, uint64_t, size_t, OSString *, OSAction **) LOCAL;`
  in `OSAction.iig` right after `Create`, using the identical `LOCAL`
  annotation so `iig` generates the same native-method scaffolding
  (`OSAction_CreateWithTypeName_Args` macro, `Create_Call`-style wrapper) it
  already does for `Create`.
- Implemented it in `IOUserServer.cpp` as a thin forwarder that ignores
  `typeName` and calls the existing `Create()` — i.e. it always allocates a
  base `OSAction` rather than the named subclass, matching this xnu's
  existing (also subclass-oblivious) `Create` behavior.
- **Follow-up (link-time):** compiling wasn't the whole story. `iig` also
  generates `OSAction::MetaClass::Dispatch()`, a uniform RPC entry point with
  an unconditional case for every declared method — LOCAL ones included —
  calling `<Method>_Impl`. That reference (`OSAction::CreateWithTypeName_Impl`,
  used from `OSAction::MetaClass::Dispatch(IORPC)`) only shows up as an
  **undefined symbol at final kernel link**, not a compile error, since
  `Dispatch` lives in generated code compiled separately from our hand-written
  `IOUserServer.cpp`. Added `IMPL(OSAction, CreateWithTypeName)` (expands to
  `OSAction::CreateWithTypeName_Impl(OSAction_CreateWithTypeName_Args)` via
  the `IMPL`/`DEFN` macros in `OSObject.iig`) right after the existing
  `IMPL(OSAction, Create)`, mirroring its body exactly (ignore `typeName`,
  same alloc logic) rather than duplicating the allocation logic by hand.
