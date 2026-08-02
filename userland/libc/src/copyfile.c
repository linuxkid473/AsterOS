/* See copyfile.h: a real data copy, not the APFS clonefile() fast path. */
#include <copyfile.h>
#include <sys/clonefile.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

int
fcopyfile(int from_fd, int to_fd, copyfile_state_t state, copyfile_flags_t flags)
{
	(void)state;
	if (!(flags & COPYFILE_DATA)) {
		return 0; /* nothing else is implemented -- see copyfile.h */
	}
	char buf[8192];
	ssize_t n;
	while ((n = read(from_fd, buf, sizeof(buf))) > 0) {
		ssize_t off = 0;
		while (off < n) {
			ssize_t w = write(to_fd, buf + off, (size_t)(n - off));
			if (w < 0) {
				return -1;
			}
			off += w;
		}
	}
	return n < 0 ? -1 : 0;
}

int
copyfile(const char *from, const char *to, copyfile_state_t state, copyfile_flags_t flags)
{
	int from_fd = open(from, O_RDONLY);
	if (from_fd < 0) {
		return -1;
	}
	int to_fd = open(to, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (to_fd < 0) {
		int saved_errno = errno;
		close(from_fd);
		errno = saved_errno;
		return -1;
	}
	int rc = fcopyfile(from_fd, to_fd, state, flags);
	int saved_errno = errno;
	close(from_fd);
	close(to_fd);
	errno = saved_errno;
	return rc;
}

copyfile_state_t copyfile_state_alloc(void) { return malloc(1); }
int copyfile_state_free(copyfile_state_t state) { free(state); return 0; }

int clonefile(const char *src, const char *dst, uint32_t flags) { (void)src; (void)dst; (void)flags; errno = ENOTSUP; return -1; }
int clonefileat(int src_dirfd, const char *src, int dst_dirfd, const char *dst, uint32_t flags) { (void)src_dirfd; (void)src; (void)dst_dirfd; (void)dst; (void)flags; errno = ENOTSUP; return -1; }
int fclonefileat(int src_fd, int dst_dirfd, const char *dst, uint32_t flags) { (void)src_fd; (void)dst_dirfd; (void)dst; (void)flags; errno = ENOTSUP; return -1; }
