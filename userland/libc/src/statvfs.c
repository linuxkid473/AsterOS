/* See sys/statvfs.h: real field remapping from statfs(), not a stub. */
#include <sys/statvfs.h>
#include <sys/mount.h>
#include <limits.h>

static void
statfs_to_statvfs(const struct statfs *sf, struct statvfs *buf)
{
	buf->f_bsize = sf->f_bsize;
	buf->f_frsize = sf->f_bsize;
	buf->f_blocks = sf->f_blocks;
	buf->f_bfree = sf->f_bfree;
	buf->f_bavail = sf->f_bavail;
	buf->f_files = sf->f_files;
	buf->f_ffree = sf->f_ffree;
	buf->f_favail = sf->f_ffree;
	buf->f_fsid = (unsigned long)sf->f_fsid.val[0];
	buf->f_flag = 0;
	buf->f_namemax = NAME_MAX;
}

int
statvfs(const char *path, struct statvfs *buf)
{
	struct statfs sf;
	if (statfs(path, &sf) != 0) {
		return -1;
	}
	statfs_to_statvfs(&sf, buf);
	return 0;
}

int
fstatvfs(int fd, struct statvfs *buf)
{
	struct statfs sf;
	if (fstatfs(fd, &sf) != 0) {
		return -1;
	}
	statfs_to_statvfs(&sf, buf);
	return 0;
}
