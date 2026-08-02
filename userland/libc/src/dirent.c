#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

ssize_t sys_getdirentries64(int fd, void *buf, size_t bufsize, off_t *position);

DIR *
fdopendir(int fd)
{
	DIR *d = malloc(sizeof(DIR));
	if (!d) {
		return (void *)0;
	}
	memset(d, 0, sizeof(*d));
	d->fd = fd;
	return d;
}

DIR *
opendir(const char *path)
{
	int fd = open(path, O_RDONLY, 0);
	if (fd < 0) {
		return (void *)0;
	}
	return fdopendir(fd);
}

struct dirent *
readdir(DIR *dirp)
{
	if (dirp->bufpos >= dirp->buflen) {
		ssize_t n = sys_getdirentries64(dirp->fd, dirp->buf, sizeof(dirp->buf), &dirp->pos);
		if (n <= 0) {
			return (void *)0;
		}
		dirp->buflen = (size_t)n;
		dirp->bufpos = 0;
	}
	struct dirent *raw = (struct dirent *)(dirp->buf + dirp->bufpos);
	if (raw->d_reclen == 0) {
		return (void *)0;
	}
	/* The kernel's on-wire record is only d_reclen bytes long (a packed,
	 * variable-length encoding -- d_name is only valid/allocated through
	 * d_namlen, padded up to d_reclen), not a full fixed-size struct
	 * dirent. Copying sizeof(dirp->cur) (~1KB) unconditionally reads
	 * past the real record into whatever garbage follows in the 8KB
	 * getdirentries64 buffer, and never guarantees d_name is
	 * NUL-terminated -- callers doing strlen()/strcmp() on that would
	 * run off into unbounded garbage, which looked like ls hanging. */
	size_t copy = raw->d_reclen;
	if (copy > sizeof(dirp->cur)) {
		copy = sizeof(dirp->cur);
	}
	memcpy(&dirp->cur, raw, copy);
	if (dirp->cur.d_namlen < sizeof(dirp->cur.d_name)) {
		dirp->cur.d_name[dirp->cur.d_namlen] = 0;
	}
	dirp->bufpos += raw->d_reclen;
	return &dirp->cur;
}

int
closedir(DIR *dirp)
{
	int r = close(dirp->fd);
	free(dirp);
	return r;
}

void
rewinddir(DIR *dirp)
{
	dirp->pos = 0;
	dirp->bufpos = 0;
	dirp->buflen = 0;
	lseek(dirp->fd, 0, SEEK_SET);
}

int
dirfd(DIR *dirp)
{
	return dirp->fd;
}
