# Patch: remove obsolete `-enable-trivial-auto-var-init-zero-knowing-it-will-be-removed-from-clang`

**Component:** xnu (`makedefs/MakeInc.def`, `CFLAGS_GEN`)

**Problem:** This flag was a temporary opt-in Apple added years ago to use `-ftrivial-auto-var-init=zero` before it stabilized in upstream clang. Modern clang (21, bundled with current Xcode) has long since made that behavior permanent and removed the escape-hatch flag entirely: `clang: error: unknown argument: '-enable-trivial-auto-var-init-zero-knowing-it-will-be-removed-from-clang'`, breaking every single compile in the kernel build.

**Fix:** Deleted the line. `-ftrivial-auto-var-init=zero` itself is still passed and still works.
