#include <string.h>
#include <stdlib.h>
#include <errno.h>

void *
memcpy(void *dst, const void *src, size_t n)
{
	unsigned char *d = (unsigned char *)dst;
	const unsigned char *s = (const unsigned char *)src;
	for (size_t i = 0; i < n; i++) {
		d[i] = s[i];
	}
	return dst;
}

void *
mempcpy(void *dst, const void *src, size_t n)
{
	memcpy(dst, src, n);
	return (unsigned char *)dst + n;
}

void *
memmove(void *dst, const void *src, size_t n)
{
	unsigned char *d = (unsigned char *)dst;
	const unsigned char *s = (const unsigned char *)src;
	if (d == s || n == 0) {
		return dst;
	}
	if (d < s) {
		for (size_t i = 0; i < n; i++) {
			d[i] = s[i];
		}
	} else {
		for (size_t i = n; i > 0; i--) {
			d[i - 1] = s[i - 1];
		}
	}
	return dst;
}

void *
memset(void *dst, int c, size_t n)
{
	unsigned char *d = (unsigned char *)dst;
	for (size_t i = 0; i < n; i++) {
		d[i] = (unsigned char)c;
	}
	return dst;
}

void
memset_pattern16(void *dst, const void *pattern16, size_t n)
{
	unsigned char *d = (unsigned char *)dst;
	const unsigned char *p = (const unsigned char *)pattern16;
	for (size_t i = 0; i < n; i++) {
		d[i] = p[i % 16];
	}
}

void bzero(void *dst, size_t n) { memset(dst, 0, n); }
/* Some clang codegen paths call the internal __bzero name directly
 * rather than the public bzero() -- a real, distinct Darwin symbol,
 * not a typo (same relationship as __memcpy_chk to memcpy). */
void __bzero(void *dst, size_t n) { memset(dst, 0, n); }

int
memcmp(const void *a, const void *b, size_t n)
{
	const unsigned char *pa = (const unsigned char *)a;
	const unsigned char *pb = (const unsigned char *)b;
	for (size_t i = 0; i < n; i++) {
		if (pa[i] != pb[i]) {
			return (int)pa[i] - (int)pb[i];
		}
	}
	return 0;
}

void *
memchr(const void *s, int c, size_t n)
{
	const unsigned char *p = (const unsigned char *)s;
	for (size_t i = 0; i < n; i++) {
		if (p[i] == (unsigned char)c) {
			return (void *)(p + i);
		}
	}
	return (void *)0;
}

size_t
strlen(const char *s)
{
	size_t n = 0;
	while (s[n]) {
		n++;
	}
	return n;
}

size_t
strnlen(const char *s, size_t maxlen)
{
	size_t n = 0;
	while (n < maxlen && s[n]) {
		n++;
	}
	return n;
}

char *
strcpy(char *dst, const char *src)
{
	char *ret = dst;
	while ((*dst++ = *src++)) {
	}
	return ret;
}

char *
strncpy(char *dst, const char *src, size_t n)
{
	size_t i = 0;
	for (; i < n && src[i]; i++) {
		dst[i] = src[i];
	}
	for (; i < n; i++) {
		dst[i] = 0;
	}
	return dst;
}

char *
stpcpy(char *dst, const char *src)
{
	while ((*dst = *src)) {
		dst++;
		src++;
	}
	return dst;
}

char *
stpncpy(char *dst, const char *src, size_t n)
{
	size_t i = 0;
	for (; i < n && src[i]; i++) {
		dst[i] = src[i];
	}
	char *end = dst + i;
	for (; i < n; i++) {
		dst[i] = 0;
	}
	return end;
}

char *
strcat(char *dst, const char *src)
{
	strcpy(dst + strlen(dst), src);
	return dst;
}

char *
strncat(char *dst, const char *src, size_t n)
{
	char *end = dst + strlen(dst);
	size_t i = 0;
	for (; i < n && src[i]; i++) {
		end[i] = src[i];
	}
	end[i] = 0;
	return dst;
}

int
strcmp(const char *a, const char *b)
{
	while (*a && (*a == *b)) {
		a++;
		b++;
	}
	return (unsigned char)*a - (unsigned char)*b;
}

int
strncmp(const char *a, const char *b, size_t n)
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

static int
lower(int c)
{
	return (c >= 'A' && c <= 'Z') ? c - 'A' + 'a' : c;
}

int
strcasecmp(const char *a, const char *b)
{
	while (*a && lower((unsigned char)*a) == lower((unsigned char)*b)) {
		a++;
		b++;
	}
	return lower((unsigned char)*a) - lower((unsigned char)*b);
}

int
strncasecmp(const char *a, const char *b, size_t n)
{
	while (n && *a && lower((unsigned char)*a) == lower((unsigned char)*b)) {
		a++;
		b++;
		n--;
	}
	if (n == 0) {
		return 0;
	}
	return lower((unsigned char)*a) - lower((unsigned char)*b);
}

char *
strchr(const char *s, int c)
{
	while (*s) {
		if (*s == (char)c) {
			return (char *)s;
		}
		s++;
	}
	return (c == 0) ? (char *)s : (void *)0;
}

char *
strrchr(const char *s, int c)
{
	const char *last = (c == 0) ? s + strlen(s) : (void *)0;
	while (*s) {
		if (*s == (char)c) {
			last = s;
		}
		s++;
	}
	return (char *)last;
}

char *
strstr(const char *hay, const char *needle)
{
	size_t nl = strlen(needle);
	if (nl == 0) {
		return (char *)hay;
	}
	for (; *hay; hay++) {
		if (strncmp(hay, needle, nl) == 0) {
			return (char *)hay;
		}
	}
	return (void *)0;
}

char *
strdup(const char *s)
{
	size_t n = strlen(s) + 1;
	char *p = malloc(n);
	if (!p) {
		return (void *)0;
	}
	memcpy(p, s, n);
	return p;
}

char *
strndup(const char *s, size_t n)
{
	size_t len = 0;
	while (len < n && s[len]) {
		len++;
	}
	char *p = malloc(len + 1);
	if (!p) {
		return (void *)0;
	}
	memcpy(p, s, len);
	p[len] = 0;
	return p;
}

size_t
strspn(const char *s, const char *accept)
{
	size_t n = 0;
	while (s[n] && strchr(accept, s[n])) {
		n++;
	}
	return n;
}

size_t
strcspn(const char *s, const char *reject)
{
	size_t n = 0;
	while (s[n] && !strchr(reject, s[n])) {
		n++;
	}
	return n;
}

char *
strpbrk(const char *s, const char *accept)
{
	for (; *s; s++) {
		if (strchr(accept, *s)) {
			return (char *)s;
		}
	}
	return (void *)0;
}

char *
strtok_r(char *s, const char *delim, char **saveptr)
{
	if (!s) {
		s = *saveptr;
	}
	s += strspn(s, delim);
	if (*s == 0) {
		*saveptr = s;
		return (void *)0;
	}
	char *tok = s;
	s += strcspn(s, delim);
	if (*s) {
		*s++ = 0;
	}
	*saveptr = s;
	return tok;
}

char *
strtok(char *s, const char *delim)
{
	static char *save;
	return strtok_r(s, delim, &save);
}

char *
strsep(char **stringp, const char *delim)
{
	char *s = *stringp;
	if (!s) {
		return (void *)0;
	}
	char *tok = s;
	while (*s && !strchr(delim, *s)) {
		s++;
	}
	if (*s) {
		*s++ = 0;
		*stringp = s;
	} else {
		*stringp = (void *)0;
	}
	return tok;
}

size_t
strlcpy(char *dst, const char *src, size_t size)
{
	size_t srclen = strlen(src);
	if (size) {
		size_t n = (srclen < size - 1) ? srclen : size - 1;
		memcpy(dst, src, n);
		dst[n] = 0;
	}
	return srclen;
}

size_t
strlcat(char *dst, const char *src, size_t size)
{
	size_t dl = strlen(dst);
	if (dl >= size) {
		return dl + strlen(src);
	}
	return dl + strlcpy(dst + dl, src, size - dl);
}

static const char *const g_errlist[] = {
	[0] = "Success",
};

char *
strerror(int errnum)
{
	static char buf[32];
	(void)errnum;
	/* Minimal: numeric fallback. Extend with real per-errno strings if a
	 * checklist command turns out to need one verbatim -- see the running
	 * libc TODO log in docs/architecture.md. */
	if (errnum == 0) {
		return (char *)g_errlist[0];
	}
	buf[0] = 'E';
	buf[1] = 'r';
	buf[2] = 'r';
	buf[3] = 'n';
	buf[4] = 'o';
	buf[5] = ' ';
	int pos = 6;
	int n = errnum;
	char tmp[12];
	int tn = 0;
	if (n == 0) {
		tmp[tn++] = '0';
	}
	while (n) {
		tmp[tn++] = '0' + (n % 10);
		n /= 10;
	}
	while (tn) {
		buf[pos++] = tmp[--tn];
	}
	buf[pos] = 0;
	return buf;
}

int
strerror_r(int errnum, char *buf, size_t buflen)
{
	strlcpy(buf, strerror(errnum), buflen);
	return 0;
}

char *
strsignal(int sig)
{
	(void)sig;
	return "signal";
}

int strcoll(const char *s1, const char *s2) { return strcmp(s1, s2); }

size_t
strxfrm(char *dst, const char *src, size_t n)
{
	size_t len = strlen(src);
	if (n) {
		size_t copy = len < n - 1 ? len : n - 1;
		for (size_t i = 0; i < copy; i++) {
			dst[i] = src[i];
		}
		dst[copy] = 0;
	}
	return len;
}
