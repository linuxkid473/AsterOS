# Patch: missing comma in `kern_malloc.c` zone-name table (real upstream bug)

**Component:** xnu (`bsd/kern/kern_malloc.c`)

**Problem:** The `M_*` malloc-tag debug-name table is missing a comma between
two entries:

```c
"fdvnodedata"   /* 122 M_FD_VN_DATA */
"fddirbuf",     /* 123 M_FD_DIRBUF */
```

In C, adjacent string literals with no comma between them concatenate, so
this silently produced ONE entry, `"fdvnodedatafddirbuf"`, at index 122, and
shifted every following name in the array down by one slot relative to its
`M_*` comment label — a real, if cosmetic (debug/stats display only, e.g.
`memstat`-style tooling), bug in Apple's own source. Modern clang's
`-Wstring-concatenation` (new since this xnu version was written) catches
exactly this pattern and treats it as an error.

**Fix:** Added the missing comma. Not a warning suppression — a one-character
correctness fix.
