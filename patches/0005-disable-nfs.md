# Patch: disable NFS client/server

**Component:** xnu (`config/MASTER.x86_64`)

**Problem:** `bsd/nfs/nfs4_vnops.c` (gated by the `nfsclient`/`config_nfs4` attributes, part of the `NFS` config-list bundle) fails ~85 `-Werror` checks under clang 21 that didn't exist/error under the ~2019 clang xnu was written against: `-Wdeclaration-after-statement`, `-Wpre-c11-compat` (a `_Static_assert` inside the `FREE()` macro), `-Wimplicit-fallthrough`, and more, scattered across an 8000-line file.

**Decision:** We have no NFS/networked-root use case anywhere in this project's goals (a local, single-machine bootable shell). Rather than patching dozens of individual warnings in code we will never execute, disabled the whole `NFS` component.

**Fix:** Removed the bare `NFS` token from the `RELEASE`/`DEVELOPMENT`/`DEBUG` attribute-list definitions in `config/MASTER.x86_64` (these comment-syntax `NAME = [ ... ]` blocks are real input to Apple's `config(8)`/`doconf`, confirmed by the fact that `nfs4_vnops.c` was compiling into our build at all — it's `optional nfsclient` / `optional config_nfs4` gated in `bsd/conf/files`, both of which come from this `NFS` bundle). This drops `nfsclient`, `nfsserver`, `config_nfs4`, `config_nfs_gss` from the active attribute set for every config, so none of `bsd/nfs/*` gets compiled at all.
