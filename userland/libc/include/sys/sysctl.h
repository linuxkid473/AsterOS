/* No real sysctl(3) MIB tree in this environment -- only CTL_KERN/
 * KERN_OSRELEASE is implemented for real (backed by the same hardcoded
 * release string uname(3) uses; see src/sysctl.c), since that's the one
 * MIB ld64's Options.cpp actually queries (to infer a deployment target
 * when none was set explicitly -- dead in practice for our toolchain,
 * which always passes an explicit --target). Every other MIB honestly
 * fails with ENOTSUP. */
#ifndef _SYS_SYSCTL_H_
#define _SYS_SYSCTL_H_

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CTL_KERN        1
#define KERN_OSRELEASE  2
#define CTL_HW          6
#define HW_NCPU         3

int sysctl(int *name, unsigned int namelen, void *oldp, size_t *oldlenp, void *newp, size_t newlen);
int sysctlbyname(const char *name, void *oldp, size_t *oldlenp, void *newp, size_t newlen);

#ifdef __cplusplus
}
#endif

#endif /* _SYS_SYSCTL_H_ */
