/* Ground-truthed against src/xnu/bsd/sys/fcntl.h. */
#ifndef _FCNTL_H_
#define _FCNTL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

#define O_RDONLY   0x0000
#define O_WRONLY   0x0001
#define O_RDWR     0x0002
#define O_ACCMODE  0x0003
#define O_NONBLOCK 0x0004
#define O_APPEND   0x0008
#define O_SHLOCK   0x0010
#define O_EXLOCK   0x0020
#define O_ASYNC    0x0040
#define O_NOFOLLOW 0x0100
#define O_CREAT    0x0200
#define O_TRUNC    0x0400
#define O_EXCL     0x0800
#define O_EVTONLY  0x8000
#define O_NOCTTY   0x20000
#define O_DIRECTORY 0x100000
#define O_SYMLINK  0x200000
#define O_CLOEXEC  0x1000000
#define O_NDELAY   O_NONBLOCK

#define F_DUPFD  0
#define F_GETFD  1
#define F_SETFD  2
#define F_GETFL  3
#define F_SETFL  4
#define F_GETOWN 5
#define F_SETOWN 6
#define F_GETLK  7
#define F_SETLK  8
#define F_SETLKW 9
#define F_GETPATH 50

#define FD_CLOEXEC 1

#define AT_FDCWD (-2)
#define AT_SYMLINK_NOFOLLOW 0x0020
#define AT_SYMLINK_FOLLOW   0x0040
#define AT_REMOVEDIR        0x0080

#define F_RDLCK 1
#define F_UNLCK 2
#define F_WRLCK 3

struct flock {
	off_t l_start;
	off_t l_len;
	pid_t l_pid;
	short l_type;
	short l_whence;
};

int open(const char *path, int flags, ...);
int openat(int fd, const char *path, int flags, ...);
int creat(const char *path, mode_t mode);
int fcntl(int fd, int cmd, ...);

#ifdef __cplusplus
}
#endif

#endif /* _FCNTL_H_ */
