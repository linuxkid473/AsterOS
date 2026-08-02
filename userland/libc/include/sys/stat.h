/* struct stat layout ground-truthed against src/xnu/bsd/sys/stat.h's
 * __DARWIN_STRUCT_STAT64 (the layout used whenever __DARWIN_64_BIT_INO_T is
 * set, which it always is on x86_64) -- field-for-field, not guessed. */
#ifndef _SYS_STAT_H_
#define _SYS_STAT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <time.h>

struct stat {
	dev_t           st_dev;
	mode_t          st_mode;
	nlink_t         st_nlink;
	ino64_t         st_ino;
	uid_t           st_uid;
	gid_t           st_gid;
	dev_t           st_rdev;
	struct timespec st_atimespec;
	struct timespec st_mtimespec;
	struct timespec st_ctimespec;
	struct timespec st_birthtimespec;
	off_t           st_size;
	blkcnt_t        st_blocks;
	blksize_t       st_blksize;
	unsigned int    st_flags;
	unsigned int    st_gen;
	int             st_lspare;
	long long       st_qspare[2];
};

/* POSIX legacy scalar-time accessors, matching real Darwin's
 * sys/stat.h -- some upstream code (e.g. busybox's
 * archival/libarchive/data_extract_all.c) reads st_mtime directly. */
#define st_atime st_atimespec.tv_sec
#define st_mtime st_mtimespec.tv_sec
#define st_ctime st_ctimespec.tv_sec
#define st_birthtime st_birthtimespec.tv_sec

#define S_IFMT   0170000
#define S_IFIFO  0010000
#define S_IFCHR  0020000
#define S_IFDIR  0040000
#define S_IFBLK  0060000
#define S_IFREG  0100000
#define S_IFLNK  0120000
#define S_IFSOCK 0140000
#define S_IFWHT  0160000

#define S_ISUID  0004000
#define S_ISGID  0002000
#define S_ISVTX  0001000
#define S_IRWXU  0000700
#define S_IRUSR  0000400
#define S_IWUSR  0000200
#define S_IXUSR  0000100
#define S_IRWXG  0000070
#define S_IRGRP  0000040
#define S_IWGRP  0000020
#define S_IXGRP  0000010
#define S_IRWXO  0000007
#define S_IROTH  0000004
#define S_IWOTH  0000002
#define S_IXOTH  0000001

#define S_ISBLK(m)  (((m) & S_IFMT) == S_IFBLK)
#define S_ISCHR(m)  (((m) & S_IFMT) == S_IFCHR)
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)
#define S_ISLNK(m)  (((m) & S_IFMT) == S_IFLNK)
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)

int stat(const char *path, struct stat *sb);
int lstat(const char *path, struct stat *sb);
int fstat(int fd, struct stat *sb);
int chmod(const char *path, mode_t mode);
int fchmod(int fd, mode_t mode);
int fchmodat(int fd, const char *path, mode_t mode, int flag);
int mkdir(const char *path, mode_t mode);
int mkfifo(const char *path, mode_t mode);
int mknod(const char *path, mode_t mode, dev_t dev);
mode_t umask(mode_t cmask);

#ifdef __cplusplus
}
#endif

#endif /* _SYS_STAT_H_ */
