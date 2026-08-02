#ifndef _SYS_MOUNT_H_
#define _SYS_MOUNT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <sys/_types/_fsid_t.h>
#include <limits.h>
#include <stdint.h>

int mount(const char *type, const char *dir, int flags, void *data);

#define MFSTYPENAMELEN 16
#define MNAMELEN       1024
#define MNT_LOCAL      0x00001000

/* Ground-truthed against src/xnu/bsd/sys/mount.h's __DARWIN_STRUCT_STATFS64
 * (what `struct statfs` really is on 64-bit-ino Darwin, i.e. always, on
 * x86_64). */
struct statfs {
	uint32_t f_bsize;
	int32_t f_iosize;
	uint64_t f_blocks;
	uint64_t f_bfree;
	uint64_t f_bavail;
	uint64_t f_files;
	uint64_t f_ffree;
	fsid_t f_fsid;
	uid_t f_owner;
	uint32_t f_type;
	uint32_t f_flags;
	uint32_t f_fssubtype;
	char f_fstypename[MFSTYPENAMELEN];
	char f_mntonname[MAXPATHLEN];
	char f_mntfromname[MAXPATHLEN];
	uint32_t f_flags_ext;
	uint32_t f_reserved[7];
};

int statfs(const char *path, struct statfs *buf);
int fstatfs(int fd, struct statfs *buf);

#ifdef __cplusplus
}
#endif

#endif /* _SYS_MOUNT_H_ */
