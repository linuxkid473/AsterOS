/* No APFS copy-on-write clone support (fat16lite has no such concept).
 * clonefile() honestly reports "not supported" -- LLVM's own
 * CopyFile() already falls back to a real data copy (copyfile(), see
 * copyfile.h) when this fails, exactly as it does on real non-APFS
 * Darwin filesystems. */
#ifndef _SYS_CLONEFILE_H_
#define _SYS_CLONEFILE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

int clonefile(const char *src, const char *dst, uint32_t flags);
int clonefileat(int src_dirfd, const char *src, int dst_dirfd, const char *dst, uint32_t flags);
int fclonefileat(int src_fd, int dst_dirfd, const char *dst, uint32_t flags);

#ifdef __cplusplus
}
#endif

#endif /* _SYS_CLONEFILE_H_ */
