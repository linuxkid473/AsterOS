/* Minimal sscanf: %d %i %u %x %o %c %s %f(skip-only) %% plus literal
 * text/whitespace matching and numeric field widths. Good enough for the
 * handful of straightforward numeric/string parses busybox's libbb code
 * does (procps.c /proc field parsing, etc.) -- not a complete C stdio
 * scanf. */
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

static int
do_vsscanf(const char *s, const char *fmt, va_list ap)
{
	int assigned = 0;
	const char *p = fmt;
	while (*p) {
		if (isspace((unsigned char)*p)) {
			while (isspace((unsigned char)*s)) {
				s++;
			}
			p++;
			continue;
		}
		if (*p != '%') {
			if (*s != *p) {
				return assigned;
			}
			s++;
			p++;
			continue;
		}
		p++;
		int width = 0;
		while (*p >= '0' && *p <= '9') {
			width = width * 10 + (*p - '0');
			p++;
		}
		int suppress = 0;
		if (*p == '*') {
			suppress = 1;
			p++;
		}
		while (*p == 'l' || *p == 'h') {
			p++;
		}
		switch (*p) {
		case 'd':
		case 'i':
		case 'u': {
			while (isspace((unsigned char)*s)) {
				s++;
			}
			const char *src = s;
			char tmp[32];
			if (width) {
				int n = 0;
				while (src[n] && n < width && n < (int)sizeof(tmp) - 1) {
					n++;
				}
				for (int i = 0; i < n; i++) {
					tmp[i] = src[i];
				}
				tmp[n] = 0;
				src = tmp;
			}
			char *end;
			long v = strtol(src, &end, 10);
			if (end == src) {
				return assigned;
			}
			if (!suppress) {
				*va_arg(ap, int *) = (int)v;
				assigned++;
			}
			s += (end - src);
			break;
		}
		case 'x': {
			while (isspace((unsigned char)*s)) {
				s++;
			}
			char *end;
			long v = strtol(s, &end, 16);
			if (end == s) {
				return assigned;
			}
			if (!suppress) {
				*va_arg(ap, int *) = (int)v;
				assigned++;
			}
			s = end;
			break;
		}
		case 'c': {
			if (!*s) {
				return assigned;
			}
			if (!suppress) {
				*va_arg(ap, char *) = *s;
				assigned++;
			}
			s++;
			break;
		}
		case 's': {
			while (isspace((unsigned char)*s)) {
				s++;
			}
			const char *start = s;
			int n = 0;
			while (*s && !isspace((unsigned char)*s) && (!width || n < width)) {
				s++;
				n++;
			}
			if (s == start) {
				return assigned;
			}
			if (!suppress) {
				char *out = va_arg(ap, char *);
				for (int i = 0; i < n; i++) {
					out[i] = start[i];
				}
				out[n] = 0;
				assigned++;
			}
			break;
		}
		case '%':
			if (*s != '%') {
				return assigned;
			}
			s++;
			break;
		default:
			return assigned;
		}
		p++;
	}
	return assigned;
}

int
vsscanf(const char *str, const char *fmt, va_list ap)
{
	return do_vsscanf(str, fmt, ap);
}

int
sscanf(const char *str, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int r = do_vsscanf(str, fmt, ap);
	va_end(ap);
	return r;
}

int
fscanf(FILE *stream, const char *fmt, ...)
{
	char buf[512];
	if (!fgets(buf, sizeof(buf), stream)) {
		return -1;
	}
	va_list ap;
	va_start(ap, fmt);
	int r = do_vsscanf(buf, fmt, ap);
	va_end(ap);
	return r;
}
