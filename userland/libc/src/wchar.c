/* Wide-character string/memory primitives -- mechanical wchar_t analogues
 * of string.c's char versions. No locale/multibyte-conversion machinery
 * exists in this environment (TZ is always UTC, see time.c's own note on
 * the same kind of limitation), so the mbstate-dependent conversions and
 * wcscoll/wcsxfrm/wcsftime are honest stubs, same spirit as strtod's
 * existing stub in stdlib_misc.c -- not real hacks, just undone work. */
#include <wchar.h>
#include <wctype.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdarg.h>

size_t
wcslen(const wchar_t *s)
{
	size_t n = 0;
	while (s[n]) {
		n++;
	}
	return n;
}

int
wcscmp(const wchar_t *a, const wchar_t *b)
{
	while (*a && *a == *b) {
		a++;
		b++;
	}
	return (int)*a - (int)*b;
}

int
wcsncmp(const wchar_t *a, const wchar_t *b, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		if (a[i] != b[i]) {
			return (int)a[i] - (int)b[i];
		}
		if (a[i] == 0) {
			return 0;
		}
	}
	return 0;
}

int wcscoll(const wchar_t *a, const wchar_t *b) { return wcscmp(a, b); }

size_t
wcsxfrm(wchar_t *dst, const wchar_t *src, size_t n)
{
	size_t len = wcslen(src);
	if (n) {
		size_t copy = len < n - 1 ? len : n - 1;
		for (size_t i = 0; i < copy; i++) {
			dst[i] = src[i];
		}
		dst[copy] = 0;
	}
	return len;
}

wchar_t *
wcscpy(wchar_t *dst, const wchar_t *src)
{
	wchar_t *ret = dst;
	while ((*dst++ = *src++)) {
	}
	return ret;
}

wchar_t *
wcsncpy(wchar_t *dst, const wchar_t *src, size_t n)
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

wchar_t *
wcscat(wchar_t *dst, const wchar_t *src)
{
	wcscpy(dst + wcslen(dst), src);
	return dst;
}

wchar_t *
wcsncat(wchar_t *dst, const wchar_t *src, size_t n)
{
	wchar_t *end = dst + wcslen(dst);
	size_t i = 0;
	for (; i < n && src[i]; i++) {
		end[i] = src[i];
	}
	end[i] = 0;
	return dst;
}

wchar_t *
wcschr(const wchar_t *s, wchar_t c)
{
	for (; *s; s++) {
		if (*s == c) {
			return (wchar_t *)s;
		}
	}
	return c == 0 ? (wchar_t *)s : (wchar_t *)0;
}

wchar_t *
wcsrchr(const wchar_t *s, wchar_t c)
{
	const wchar_t *last = c == 0 ? s + wcslen(s) : (wchar_t *)0;
	for (; *s; s++) {
		if (*s == c) {
			last = s;
		}
	}
	return (wchar_t *)last;
}

wchar_t *
wcspbrk(const wchar_t *s1, const wchar_t *s2)
{
	for (; *s1; s1++) {
		if (wcschr(s2, *s1)) {
			return (wchar_t *)s1;
		}
	}
	return (wchar_t *)0;
}

wchar_t *
wcsstr(const wchar_t *s1, const wchar_t *s2)
{
	size_t n2 = wcslen(s2);
	if (n2 == 0) {
		return (wchar_t *)s1;
	}
	for (; *s1; s1++) {
		if (wcsncmp(s1, s2, n2) == 0) {
			return (wchar_t *)s1;
		}
	}
	return (wchar_t *)0;
}

size_t
wcsspn(const wchar_t *s, const wchar_t *accept)
{
	size_t n = 0;
	while (s[n] && wcschr(accept, s[n])) {
		n++;
	}
	return n;
}

size_t
wcscspn(const wchar_t *s, const wchar_t *reject)
{
	size_t n = 0;
	while (s[n] && !wcschr(reject, s[n])) {
		n++;
	}
	return n;
}

wchar_t *
wcstok(wchar_t *s, const wchar_t *delim, wchar_t **saveptr)
{
	if (!s) {
		s = *saveptr;
	}
	s += wcsspn(s, delim);
	if (!*s) {
		*saveptr = s;
		return (wchar_t *)0;
	}
	wchar_t *tok = s;
	s += wcscspn(s, delim);
	if (*s) {
		*s++ = 0;
	}
	*saveptr = s;
	return tok;
}

wchar_t *
wmemchr(const wchar_t *s, wchar_t c, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		if (s[i] == c) {
			return (wchar_t *)(s + i);
		}
	}
	return (wchar_t *)0;
}

int
wmemcmp(const wchar_t *a, const wchar_t *b, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		if (a[i] != b[i]) {
			return (int)a[i] - (int)b[i];
		}
	}
	return 0;
}

wchar_t *
wmemcpy(wchar_t *dst, const wchar_t *src, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		dst[i] = src[i];
	}
	return dst;
}

wchar_t *
wmemmove(wchar_t *dst, const wchar_t *src, size_t n)
{
	if (dst < src) {
		for (size_t i = 0; i < n; i++) {
			dst[i] = src[i];
		}
	} else if (dst > src) {
		for (size_t i = n; i > 0; i--) {
			dst[i - 1] = src[i - 1];
		}
	}
	return dst;
}

wchar_t *
wmemset(wchar_t *dst, wchar_t c, size_t n)
{
	for (size_t i = 0; i < n; i++) {
		dst[i] = c;
	}
	return dst;
}

/* TODO: no real numeric-parsing or mbstate/locale machinery implemented
 * yet -- honest stubs, same as strtod's existing one in stdlib_misc.c. */
double wcstod(const wchar_t *s, wchar_t **endptr) { (void)s; if (endptr) { *endptr = (wchar_t *)s; } return 0.0; }
float wcstof(const wchar_t *s, wchar_t **endptr) { (void)s; if (endptr) { *endptr = (wchar_t *)s; } return 0.0f; }
long double wcstold(const wchar_t *s, wchar_t **endptr) { (void)s; if (endptr) { *endptr = (wchar_t *)s; } return 0.0L; }
long wcstol(const wchar_t *s, wchar_t **endptr, int base) { (void)s; (void)base; if (endptr) { *endptr = (wchar_t *)s; } return 0; }
long long wcstoll(const wchar_t *s, wchar_t **endptr, int base) { (void)s; (void)base; if (endptr) { *endptr = (wchar_t *)s; } return 0; }
unsigned long wcstoul(const wchar_t *s, wchar_t **endptr, int base) { (void)s; (void)base; if (endptr) { *endptr = (wchar_t *)s; } return 0; }
unsigned long long wcstoull(const wchar_t *s, wchar_t **endptr, int base) { (void)s; (void)base; if (endptr) { *endptr = (wchar_t *)s; } return 0; }

size_t wcsftime(wchar_t *s, size_t maxsize, const wchar_t *format, const struct tm *timeptr) { (void)format; (void)timeptr; if (maxsize) { s[0] = 0; } return 0; }

size_t wcsrtombs(char *dst, const wchar_t **src, size_t n, mbstate_t *ps) { (void)src; (void)n; (void)ps; if (dst && n) { dst[0] = 0; } return 0; }
size_t wcsnrtombs(char *dst, const wchar_t **src, size_t nwc, size_t n, mbstate_t *ps) { (void)src; (void)nwc; (void)n; (void)ps; if (dst && n) { dst[0] = 0; } return 0; }

/* ASCII-only multibyte<->wide conversions: one byte is one wchar_t, no
 * shift state ever needed (mbstate_t is unused/untouched throughout --
 * consistent with this codebase having no real multibyte encoding
 * support anywhere, see time.c's UTC-only note for the same kind of
 * limitation). */
wint_t btowc(int c) { return c == -1 /* EOF */ ? WEOF : (wint_t)(unsigned char)c; }
int wctob(wint_t wc) { return wc == WEOF || wc > 0xff ? -1 /* EOF */ : (int)wc; }

wint_t fputwc(wchar_t c, FILE *stream) { int r = fputc((int)c, stream); return r == -1 /* EOF */ ? WEOF : (wint_t)r; }
wint_t fgetwc(FILE *stream) { int r = fgetc(stream); return r == -1 /* EOF */ ? WEOF : (wint_t)r; }
wint_t getwc(FILE *stream) { return fgetwc(stream); }
wint_t putwc(wchar_t c, FILE *stream) { return fputwc(c, stream); }
wint_t ungetwc(wint_t c, FILE *stream) { int r = ungetc((int)c, stream); return r == -1 /* EOF */ ? WEOF : (wint_t)r; }

int
mbtowc(wchar_t *pwc, const char *s, size_t n)
{
	if (!s) {
		return 0; /* not state-dependent */
	}
	if (n == 0) {
		return -1;
	}
	if (pwc) {
		*pwc = (wchar_t)(unsigned char)*s;
	}
	return *s ? 1 : 0;
}

size_t
mbrtowc(wchar_t *pwc, const char *s, size_t n, mbstate_t *ps)
{
	(void)ps;
	if (!s) {
		return 0;
	}
	if (n == 0) {
		return (size_t)-2;
	}
	if (pwc) {
		*pwc = (wchar_t)(unsigned char)*s;
	}
	return *s ? 1 : 0;
}

size_t mbrlen(const char *s, size_t n, mbstate_t *ps) { return mbrtowc((wchar_t *)0, s, n, ps); }

size_t
wcrtomb(char *s, wchar_t wc, mbstate_t *ps)
{
	(void)ps;
	if (!s) {
		return 1;
	}
	s[0] = (char)wc;
	return 1;
}

size_t
mbsrtowcs(wchar_t *dst, const char **src, size_t n, mbstate_t *ps)
{
	(void)ps;
	const char *s = *src;
	size_t i = 0;
	for (; (!dst || i < n) && s[i]; i++) {
		if (dst) {
			dst[i] = (wchar_t)(unsigned char)s[i];
		}
	}
	if (dst) {
		if (s[i] == 0 && i < n) {
			dst[i] = 0;
			*src = (const char *)0;
		} else {
			*src = s + i;
		}
	}
	return i;
}

size_t
mbsnrtowcs(wchar_t *dst, const char **src, size_t nms, size_t n, mbstate_t *ps)
{
	(void)ps;
	const char *s = *src;
	size_t i = 0;
	for (; i < nms && (!dst || i < n) && s[i]; i++) {
		if (dst) {
			dst[i] = (wchar_t)(unsigned char)s[i];
		}
	}
	if (dst) {
		if (i < n) {
			dst[i] = 0;
		}
		*src = s + i;
	}
	return i;
}

/* ASCII-only: narrow the (wide) format string, format with the real
 * narrow vsnprintf, then widen the result byte-for-byte -- consistent
 * with this codebase having no real multibyte/locale support anywhere
 * (see time.c's UTC-only note for the same kind of limitation). Fixed
 * intermediate buffer sizes, not a fully general implementation. */
int
vswprintf(wchar_t *s, size_t n, const wchar_t *format, va_list ap)
{
	char narrow_fmt[256];
	size_t i = 0;
	for (; format[i] && i < sizeof(narrow_fmt) - 1; i++) {
		narrow_fmt[i] = (char)format[i];
	}
	narrow_fmt[i] = 0;

	char buf[1024];
	int written = vsnprintf(buf, sizeof(buf), narrow_fmt, ap);
	if (written < 0) {
		return written;
	}
	size_t copy = (size_t)written < n ? (size_t)written : (n ? n - 1 : 0);
	for (i = 0; i < copy; i++) {
		s[i] = (wchar_t)buf[i];
	}
	if (n) {
		s[copy] = 0;
	}
	return written;
}

int
swprintf(wchar_t *s, size_t n, const wchar_t *format, ...)
{
	va_list ap;
	va_start(ap, format);
	int r = vswprintf(s, n, format, ap);
	va_end(ap);
	return r;
}

/* Wide-character classification: this environment only ever deals in
 * ASCII (see the note above), so these are exactly the plain ctype.h
 * classifiers on a wider argument type. */
int iswalpha(wint_t wc) { return isalpha((int)wc); }
int iswdigit(wint_t wc) { return isdigit((int)wc); }
int iswalnum(wint_t wc) { return isalnum((int)wc); }
int iswspace(wint_t wc) { return isspace((int)wc); }
int iswupper(wint_t wc) { return isupper((int)wc); }
int iswlower(wint_t wc) { return islower((int)wc); }
int iswcntrl(wint_t wc) { return iscntrl((int)wc); }
int iswprint(wint_t wc) { return isprint((int)wc); }
int iswgraph(wint_t wc) { return isgraph((int)wc); }
int iswpunct(wint_t wc) { return ispunct((int)wc); }
int iswxdigit(wint_t wc) { return isxdigit((int)wc); }
int iswblank(wint_t wc) { return isblank((int)wc); }
wint_t towupper(wint_t wc) { return (wint_t)toupper((int)wc); }
wint_t towlower(wint_t wc) { return (wint_t)tolower((int)wc); }

/* wctype()/iswctype(): the "name a classification, test it later"
 * indirection form of the iswXXX functions above. Only the standard
 * class names are recognized. */
enum {
	__WCTYPE_ALNUM = 1, __WCTYPE_ALPHA, __WCTYPE_BLANK, __WCTYPE_CNTRL,
	__WCTYPE_DIGIT, __WCTYPE_GRAPH, __WCTYPE_LOWER, __WCTYPE_PRINT,
	__WCTYPE_PUNCT, __WCTYPE_SPACE, __WCTYPE_UPPER, __WCTYPE_XDIGIT,
};

wctype_t
wctype(const char *name)
{
	static const char *const names[] = {
		"", "alnum", "alpha", "blank", "cntrl", "digit", "graph",
		"lower", "print", "punct", "space", "upper", "xdigit",
	};
	for (size_t i = 1; i < sizeof(names) / sizeof(names[0]); i++) {
		if (strcmp(name, names[i]) == 0) {
			return (wctype_t)i;
		}
	}
	return (wctype_t)0;
}

int
iswctype(wint_t wc, wctype_t desc)
{
	switch (desc) {
	case __WCTYPE_ALNUM: return iswalnum(wc);
	case __WCTYPE_ALPHA: return iswalpha(wc);
	case __WCTYPE_BLANK: return iswblank(wc);
	case __WCTYPE_CNTRL: return iswcntrl(wc);
	case __WCTYPE_DIGIT: return iswdigit(wc);
	case __WCTYPE_GRAPH: return iswgraph(wc);
	case __WCTYPE_LOWER: return iswlower(wc);
	case __WCTYPE_PRINT: return iswprint(wc);
	case __WCTYPE_PUNCT: return iswpunct(wc);
	case __WCTYPE_SPACE: return iswspace(wc);
	case __WCTYPE_UPPER: return iswupper(wc);
	case __WCTYPE_XDIGIT: return iswxdigit(wc);
	default: return 0;
	}
}
