# Patch: port `makekdebugevents.py` to Python 3

**Component:** xnu (`bsd/kern/makekdebugevents.py`)

**Problem:** `bsd/sys/Makefile`'s `kdebugevents.h` generation rule (required for
DEVELOPMENT/DEBUG kernel configs) invokes this script directly via its shebang.
Two independent problems on this host:
1. The shebang is `#!/usr/bin/python` — this macOS has no Python 2 at all, only
   `/usr/bin/python3`. Fails with `bad interpreter: No such file or directory`.
2. The script itself is Python-2-only syntax (`print "..."` statement form,
   plus a tab/space indentation mix that's a hard error under Python 3).

**Fix:** Changed the shebang to `#!/usr/bin/env python3` and ported the ~35-line
script to Python 3 (`print(...)` calls, raw string for the regex, consistent
indentation). Logic is unchanged — verified output matches the original format
(a `kd_event_t kd_events[]` C table) against a small hand-written test input.

Two other scripts in this tree (`tools/tests/perf_index/test_controller.py`,
`tools/lldbmacros/core/operating_system.py`) have the same broken shebang but
are outside our build path (test infra, not invoked by `make ... exporthdrs`/`all`)
— left alone.

**Follow-up:** `tools/lldbmacros/core/syntax_checker.py` turned out to be on
the build path after all — the final `make all` (once the kernel itself had
linked, see patch 0013) runs it once per installed LLDB-macro Python file via
`makedefs/MakeInc.rule`'s `INSTALLPYTHON_RULE_template` (`$(PYTHON)
.../syntax_checker.py <file>`, where `$(PYTHON)` is never actually defined
anywhere in this xnu tree, so make expands it to nothing and the script runs
directly via its own shebang). Same two problems as `makekdebugevents.py`:
`#!/usr/bin/env python` (no Python 2 on this host) and genuine Python-2-only
syntax (`print >>sys.stderr, ...` / bare `print helpdoc` statements). Fixed
the same way: `#!/usr/bin/env python3` plus porting the ~50-line script to
Python 3 print-function syntax; logic unchanged.

A second, unrelated version-skew issue surfaced right after: this Python
install (3.14) raises `FileExistsError` from `py_compile.compile(fname,
cfile="/dev/null", doraise=True)` ("/dev/null is a non-regular file and will
be changed into a regular one if import writes a byte-compiled file to it")
— a new safety check, not present in older Python 3 releases this script was
last exercised against. Since the script only ever wanted a syntax check,
not an actual bytecode artifact, replaced that call with
`compile(open(fname).read(), fname, 'exec')` (the plain builtin, catching
`SyntaxError` instead of `py_compile.PyCompileError`) — checks syntax
without writing anything anywhere, sidestepping the new restriction entirely
rather than fighting it.
