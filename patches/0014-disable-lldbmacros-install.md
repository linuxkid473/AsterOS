# Patch: skip the lldbmacros install step

**Component:** xnu (`Makefile`)

**Problem:** `make all`'s final phase (`config_all`, run once the kernel
itself has already fully compiled and linked — see patch 0013) recurses into
`tools/` whose entire purpose (`tools/Makefile`: `CONFIG_SUBDIRS =
lldbmacros`) is installing `tools/lldbmacros/*.py` — Python scripts that
teach `lldb` how to introspect a live/crashed kernel interactively (`showtask`,
`zprint`, etc. macros). Each installed file is run through
`tools/lldbmacros/core/syntax_checker.py` as a syntax gate (fixed for Python 3
in patch 0008), which then correctly reports that **39 of the tree's 60
Python files are themselves Python-2-only** (`print` statements, etc.) —
this is Apple's own debugging tooling, last updated for Python 2, not
something introduced by our changes.

**Decision:** This project has no interactive kernel-debugging use case (no
one is attaching `lldb` to a live/crashed instance of this kernel) — porting
dozens of debugging-macro files to Python 3 for a capability we don't use
would be pure scope creep. The kernel binary itself doesn't need this step at
all; it's packaging for a separate, optional developer tool.

**Fix:** Removed `tools` from `CONFIG_SUBDIRS` in the top-level `Makefile`
(`config tools san` → `config san`). `tools/Makefile`'s own `CONFIG_SUBDIRS`
already resolves to just `lldbmacros`, so this has the identical effect to
excluding that one subdirectory, via a single-line change at the point where
it's selected rather than editing `tools/Makefile` itself.
