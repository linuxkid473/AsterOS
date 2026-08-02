# Patch: disable netboot (config_netboot)

**Component:** xnu (`config/MASTER.x86_64`)

**Problem:** With NFS disabled (patch 0005), the final kernel link failed:
```
Undefined symbols for architecture x86_64:
  "_nfs_mountroot", referenced from: _netboot_mountroot in lto.o
```
`bsd/kern/netboot.c` (`optional config_netboot` in `bsd/conf/files`, part of
the always-on `BSD_BASE` bundle) calls `nfs_mountroot()` unconditionally —
netboot's entire purpose is booting root over the network via NFS, so it's
dead weight without NFS regardless of the undefined symbol.

**Decision:** We have no network-boot use case in this project (a local,
single-machine bootable shell), so netboot goes the same way NFS did.

**Fix:** Removed `config_netboot` from `BSD_BASE` in `config/MASTER.x86_64`,
same mechanism as patch 0005.
