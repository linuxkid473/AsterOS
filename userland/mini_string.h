/* Minimal freestanding string/memory helpers -- no libc available. */
#ifndef DARWINBUILD_MINI_STRING_H
#define DARWINBUILD_MINI_STRING_H

#include "syscall.h"

static inline size_t
xstrlen(const char *s)
{
	size_t n = 0;
	while (s[n]) {
		n++;
	}
	return n;
}

static inline int
xstrcmp(const char *a, const char *b)
{
	while (*a && (*a == *b)) {
		a++;
		b++;
	}
	return (unsigned char)*a - (unsigned char)*b;
}

static inline int
xstrncmp(const char *a, const char *b, size_t n)
{
	while (n && *a && (*a == *b)) {
		a++;
		b++;
		n--;
	}
	if (n == 0) {
		return 0;
	}
	return (unsigned char)*a - (unsigned char)*b;
}

static inline void
xmemcpy(void *dst, const void *src, size_t n)
{
	unsigned char *d = (unsigned char *)dst;
	const unsigned char *s = (const unsigned char *)src;
	for (size_t i = 0; i < n; i++) {
		d[i] = s[i];
	}
}

static inline void
xmemset(void *dst, int c, size_t n)
{
	unsigned char *d = (unsigned char *)dst;
	for (size_t i = 0; i < n; i++) {
		d[i] = (unsigned char)c;
	}
}

/* Copies at most (cap-1) bytes and always null-terminates -- unlike BSD
 * strlcpy this doesn't return the source length (we never need it), which
 * keeps every call site simpler. */
static inline void
xstrcpy(char *dst, const char *src, size_t cap)
{
	size_t i = 0;
	while (src[i] && i + 1 < cap) {
		dst[i] = src[i];
		i++;
	}
	dst[i] = 0;
}

static inline void
xstrcat(char *dst, const char *src, size_t cap)
{
	size_t dl = xstrlen(dst);
	if (dl + 1 < cap) {
		xstrcpy(dst + dl, src, cap - dl);
	}
}

#endif /* DARWINBUILD_MINI_STRING_H */
