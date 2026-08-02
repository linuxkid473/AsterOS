/* POSIX statvfs()/fstatvfs() -- a thin field-remapping wrapper over our
 * real BSD statfs()/fstatfs() (sys/mount.h), same relationship these
 * have on real BSD-derived Darwin. */
#ifndef _SYS_STATVFS_H_
#define _SYS_STATVFS_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long fsblkcnt_t;
typedef unsigned long fsfilcnt_t;

struct statvfs {
	unsigned long f_bsize;
	unsigned long f_frsize;
	fsblkcnt_t f_blocks;
	fsblkcnt_t f_bfree;
	fsblkcnt_t f_bavail;
	fsfilcnt_t f_files;
	fsfilcnt_t f_ffree;
	fsfilcnt_t f_favail;
	unsigned long f_fsid;
	unsigned long f_flag;
	unsigned long f_namemax;
};

#define ST_RDONLY 0x00000001
#define ST_NOSUID 0x00000002

int statvfs(const char *path, struct statvfs *buf);
int fstatvfs(int fd, struct statvfs *buf);

#ifdef __cplusplus
}
#endif

#endif /* _SYS_STATVFS_H_ */
