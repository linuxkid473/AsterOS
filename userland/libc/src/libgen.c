/* POSIX basename(3)/dirname(3) -- real Darwin's versions return a
 * pointer into an internal per-call static buffer (not into the input
 * string), which is what callers (e.g. ld64's Snapshot.cpp) rely on
 * since they may pass a string literal or later mutate `path`. */
#include <libgen.h>
#include <string.h>
#include <limits.h>

char *
basename(char *path)
{
	static char buf[PATH_MAX];
	if (!path || !*path) {
		return (char *)".";
	}
	size_t len = strlen(path);
	while (len > 1 && path[len - 1] == '/') {
		len--;
	}
	size_t start = len;
	while (start > 0 && path[start - 1] != '/') {
		start--;
	}
	if (start == len) {
		return (char *)"/";
	}
	size_t n = len - start;
	if (n >= sizeof(buf)) {
		n = sizeof(buf) - 1;
	}
	memcpy(buf, path + start, n);
	buf[n] = 0;
	return buf;
}

char *
dirname(char *path)
{
	static char buf[PATH_MAX];
	if (!path || !*path) {
		return (char *)".";
	}
	size_t len = strlen(path);
	while (len > 1 && path[len - 1] == '/') {
		len--;
	}
	while (len > 0 && path[len - 1] != '/') {
		len--;
	}
	while (len > 1 && path[len - 1] == '/') {
		len--;
	}
	if (len == 0) {
		return (char *)".";
	}
	if (len >= sizeof(buf)) {
		len = sizeof(buf) - 1;
	}
	memcpy(buf, path, len);
	buf[len] = 0;
	return buf;
}
