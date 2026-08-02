/* config.h for compiler-rt's vendored BlocksRuntime (src/llvm-project/
 * compiler-rt/lib/BlocksRuntime) -- normally produced by that project's
 * own build probing for available atomics/headers. We select the plain
 * GCC/clang __sync builtin path rather than the deprecated userspace
 * OSAtomicCompareAndSwap* API (which the current SDK headers no longer
 * declare -- only the kernel-side libkern/OSAtomic.h names survive).
 */
#ifndef _LD64_BLOCKSRUNTIME_CONFIG_H_
#define _LD64_BLOCKSRUNTIME_CONFIG_H_

#define HAVE_SYNC_BOOL_COMPARE_AND_SWAP_INT 1
#define HAVE_SYNC_BOOL_COMPARE_AND_SWAP_LONG 1

#endif
