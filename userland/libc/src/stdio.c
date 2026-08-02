/* Minimal FILE* stdio -- our own struct, not Apple's struct __sFILE ABI.
 * Unbuffered writes, small read-ahead buffer for fgetc/fgets. Enough for
 * busybox applets that use fprintf/fputs/fgets/perror -- not a complete
 * ISO C stdio implementation. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>

static FILE g_stdin = { .fd = 0, .ungot = -1 };
static FILE g_stdout = { .fd = 1, .ungot = -1 };
static FILE g_stderr = { .fd = 2, .ungot = -1 };
FILE *stdin = &g_stdin;
FILE *stdout = &g_stdout;
FILE *stderr = &g_stderr;

FILE *
fdopen(int fd, const char *mode)
{
	(void)mode;
	FILE *f = malloc(sizeof(FILE));
	if (!f) {
		return (void *)0;
	}
	memset(f, 0, sizeof(*f));
	f->fd = fd;
	f->ungot = -1;
	return f;
}

FILE *
fopen(const char *path, const char *mode)
{
	int flags = 0;
	if (strcmp(mode, "r") == 0 || strcmp(mode, "rb") == 0) {
		flags = O_RDONLY;
	} else if (strcmp(mode, "w") == 0 || strcmp(mode, "wb") == 0) {
		flags = O_WRONLY | O_CREAT | O_TRUNC;
	} else if (strcmp(mode, "a") == 0 || strcmp(mode, "ab") == 0) {
		flags = O_WRONLY | O_CREAT | O_APPEND;
	} else if (strcmp(mode, "r+") == 0) {
		flags = O_RDWR;
	} else if (strcmp(mode, "w+") == 0) {
		flags = O_RDWR | O_CREAT | O_TRUNC;
	} else {
		flags = O_RDONLY;
	}
	int fd = open(path, flags, 0666);
	if (fd < 0) {
		return (void *)0;
	}
	return fdopen(fd, mode);
}

FILE *
freopen(const char *path, const char *mode, FILE *stream)
{
	FILE *n = fopen(path, mode);
	if (!n) {
		return (void *)0;
	}
	close(stream->fd);
	stream->fd = n->fd;
	stream->eof = 0;
	stream->error = 0;
	stream->ungot = -1;
	free(n);
	return stream;
}

int
fclose(FILE *stream)
{
	int r = close(stream->fd);
	if (stream != &g_stdin && stream != &g_stdout && stream != &g_stderr) {
		free(stream);
	}
	return r;
}

int
fflush(FILE *stream)
{
	(void)stream;
	return 0; /* unbuffered writes -- nothing to flush */
}

/* Every stream here is already unbuffered (one syscall per read/write,
 * see fgetc/fwrite) -- these are honestly no-ops, not shortcuts: there
 * is no buffer to configure. */
void setbuf(FILE *stream, char *buf) { (void)stream; (void)buf; }
int setvbuf(FILE *stream, char *buf, int mode, size_t size) { (void)stream; (void)buf; (void)mode; (void)size; return 0; }

int
feof(FILE *stream)
{
	return stream->eof;
}

int
ferror(FILE *stream)
{
	return stream->error;
}

void
clearerr(FILE *stream)
{
	stream->eof = 0;
	stream->error = 0;
}

int
fileno(FILE *stream)
{
	return stream->fd;
}

long
ftell(FILE *stream)
{
	return lseek(stream->fd, 0, SEEK_CUR);
}

int
fseek(FILE *stream, long offset, int whence)
{
	stream->ungot = -1;
	stream->eof = 0;
	return (lseek(stream->fd, offset, whence) < 0) ? -1 : 0;
}

void
rewind(FILE *stream)
{
	fseek(stream, 0, SEEK_SET);
}

int
fseeko(FILE *stream, off_t offset, int whence)
{
	stream->ungot = -1;
	stream->eof = 0;
	return (lseek(stream->fd, offset, whence) < 0) ? -1 : 0;
}

off_t
ftello(FILE *stream)
{
	return lseek(stream->fd, 0, SEEK_CUR);
}

int
fgetc(FILE *stream)
{
	if (stream->ungot >= 0) {
		int c = stream->ungot;
		stream->ungot = -1;
		return c;
	}
	unsigned char c;
	ssize_t n = read(stream->fd, &c, 1);
	if (n == 0) {
		stream->eof = 1;
		return EOF;
	}
	if (n < 0) {
		stream->error = 1;
		return EOF;
	}
	return c;
}

int getc(FILE *stream) { return fgetc(stream); }
int getchar(void) { return fgetc(stdin); }

int
ungetc(int c, FILE *stream)
{
	if (c == EOF) {
		return EOF;
	}
	stream->ungot = c;
	stream->eof = 0;
	return c;
}

char *
fgets(char *s, int size, FILE *stream)
{
	int i = 0;
	if (size <= 0) {
		return (void *)0;
	}
	while (i < size - 1) {
		int c = fgetc(stream);
		if (c == EOF) {
			if (i == 0) {
				return (void *)0;
			}
			break;
		}
		s[i++] = (char)c;
		if (c == '\n') {
			break;
		}
	}
	s[i] = 0;
	return s;
}

int
fputc(int c, FILE *stream)
{
	unsigned char ch = (unsigned char)c;
	if (write(stream->fd, &ch, 1) != 1) {
		stream->error = 1;
		return EOF;
	}
	return (int)ch;
}

int putc(int c, FILE *stream) { return fputc(c, stream); }
int putchar(int c) { return fputc(c, stdout); }

int
fputs(const char *s, FILE *stream)
{
	size_t n = strlen(s);
	ssize_t w = write(stream->fd, s, n);
	if (w < 0 || (size_t)w != n) {
		stream->error = 1;
		return EOF;
	}
	return 0;
}

int
puts(const char *s)
{
	if (fputs(s, stdout) == EOF) {
		return EOF;
	}
	return fputc('\n', stdout) == EOF ? EOF : 0;
}

size_t
fread(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t total = size * nmemb;
	size_t got = 0;
	unsigned char *p = ptr;
	while (got < total) {
		int c = fgetc(stream);
		if (c == EOF) {
			break;
		}
		p[got++] = (unsigned char)c;
	}
	return (size == 0) ? 0 : got / size;
}

size_t
fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t total = size * nmemb;
	ssize_t w = write(stream->fd, ptr, total);
	if (w < 0) {
		stream->error = 1;
		return 0;
	}
	return (size == 0) ? 0 : (size_t)w / size;
}

/* ---- minimal printf family: %d %i %u %x %X %o %c %s %p %% with an
 * optional zero-pad width (e.g. %02d) -- covers what busybox's own
 * applets (as opposed to libbb's bb_* helpers, which mostly avoid libc
 * stdio) actually format through this path. */
static int
out_str(char **bufp, size_t *remp, int fd, const char *s, size_t n)
{
	if (fd >= 0) {
		if (write(fd, s, n) != (ssize_t)n) {
			return -1;
		}
		return (int)n;
	}
	size_t take = (n < *remp) ? n : *remp;
	memcpy(*bufp, s, take);
	*bufp += take;
	*remp -= take;
	return (int)n;
}

static int
vformat(int fd, char *buf, size_t bufsize, const char *fmt, va_list ap)
{
	size_t rem = bufsize;
	char *p = buf;
	int total = 0;
	char numbuf[32];

	for (; *fmt; fmt++) {
		if (*fmt != '%') {
			int r = out_str(&p, &rem, fd, fmt, 1);
			if (r < 0) {
				return -1;
			}
			total += r;
			continue;
		}
		fmt++;
		int zero_pad = 0;
		int left_justify = 0;
		int width = 0;
		for (;;) {
			if (*fmt == '0') {
				zero_pad = 1;
				fmt++;
			} else if (*fmt == '-') {
				left_justify = 1;
				fmt++;
			} else {
				break;
			}
		}
		if (*fmt == '*') {
			/* width taken from the next vararg (used by busybox's own
			 * column-formatted output, e.g. ls) -- was previously
			 * unhandled, falling into the unrecognized-spec default
			 * case and printing garbage like "*s" literally. */
			width = va_arg(ap, int);
			if (width < 0) {
				left_justify = 1;
				width = -width;
			}
			fmt++;
		} else {
			while (*fmt >= '0' && *fmt <= '9') {
				width = width * 10 + (*fmt - '0');
				fmt++;
			}
		}
		/* skip length modifiers we don't distinguish (treat as long) */
		int is_long = 0;
		while (*fmt == 'l' || *fmt == 'z' || *fmt == 'h') {
			if (*fmt == 'l') {
				is_long++;
			}
			fmt++;
		}

		switch (*fmt) {
		case 'd':
		case 'i': {
			long v = is_long ? va_arg(ap, long) : va_arg(ap, int);
			int neg = v < 0;
			unsigned long uv = neg ? (unsigned long)(-v) : (unsigned long)v;
			int n = 0;
			if (uv == 0) {
				numbuf[n++] = '0';
			}
			while (uv) {
				numbuf[n++] = '0' + (uv % 10);
				uv /= 10;
			}
			int len = n + (neg ? 1 : 0);
			for (int pad = len; pad < width; pad++) {
				out_str(&p, &rem, fd, zero_pad ? "0" : " ", 1);
				total++;
			}
			if (neg) {
				out_str(&p, &rem, fd, "-", 1);
				total++;
			}
			while (n) {
				char c = numbuf[--n];
				out_str(&p, &rem, fd, &c, 1);
				total++;
			}
			break;
		}
		case 'u':
		case 'x':
		case 'X':
		case 'o': {
			unsigned long uv = is_long ? va_arg(ap, unsigned long) : va_arg(ap, unsigned int);
			int base = (*fmt == 'o') ? 8 : (*fmt == 'u') ? 10 : 16;
			const char *digits = (*fmt == 'X') ? "0123456789ABCDEF" : "0123456789abcdef";
			int n = 0;
			if (uv == 0) {
				numbuf[n++] = '0';
			}
			while (uv) {
				numbuf[n++] = digits[uv % base];
				uv /= base;
			}
			for (int pad = n; pad < width; pad++) {
				out_str(&p, &rem, fd, zero_pad ? "0" : " ", 1);
				total++;
			}
			while (n) {
				char c = numbuf[--n];
				out_str(&p, &rem, fd, &c, 1);
				total++;
			}
			break;
		}
		case 'c': {
			char c = (char)va_arg(ap, int);
			out_str(&p, &rem, fd, &c, 1);
			total++;
			break;
		}
		case 's': {
			const char *s = va_arg(ap, const char *);
			if (!s) {
				s = "(null)";
			}
			size_t len = strlen(s);
			if (!left_justify) {
				for (size_t pad = len; pad < (size_t)width; pad++) {
					out_str(&p, &rem, fd, " ", 1);
					total++;
				}
			}
			out_str(&p, &rem, fd, s, len);
			total += (int)len;
			if (left_justify) {
				for (size_t pad = len; pad < (size_t)width; pad++) {
					out_str(&p, &rem, fd, " ", 1);
					total++;
				}
			}
			break;
		}
		case 'p': {
			unsigned long v = (unsigned long)va_arg(ap, void *);
			out_str(&p, &rem, fd, "0x", 2);
			total += 2;
			int n = 0;
			if (v == 0) {
				numbuf[n++] = '0';
			}
			while (v) {
				numbuf[n++] = "0123456789abcdef"[v % 16];
				v /= 16;
			}
			while (n) {
				char c = numbuf[--n];
				out_str(&p, &rem, fd, &c, 1);
				total++;
			}
			break;
		}
		case '%': {
			char c = '%';
			out_str(&p, &rem, fd, &c, 1);
			total++;
			break;
		}
		case 0:
			fmt--;
			break;
		default: {
			char c = *fmt;
			out_str(&p, &rem, fd, &c, 1);
			total++;
			break;
		}
		}
	}
	if (fd < 0 && bufsize > 0) {
		*p = 0;
	}
	return total;
}

int
vfprintf(FILE *stream, const char *fmt, va_list ap)
{
	return vformat(stream->fd, (void *)0, 0, fmt, ap);
}

int
vprintf(const char *fmt, va_list ap)
{
	return vfprintf(stdout, fmt, ap);
}

int
vsnprintf(char *str, size_t size, const char *fmt, va_list ap)
{
	return vformat(-1, str, size, fmt, ap);
}

int
vsprintf(char *str, const char *fmt, va_list ap)
{
	return vformat(-1, str, (size_t)-1, fmt, ap);
}

int
fprintf(FILE *stream, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int r = vfprintf(stream, fmt, ap);
	va_end(ap);
	return r;
}

int
printf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int r = vprintf(fmt, ap);
	va_end(ap);
	return r;
}

int
sprintf(char *str, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int r = vsprintf(str, fmt, ap);
	va_end(ap);
	return r;
}

int
vasprintf(char **strp, const char *fmt, va_list ap)
{
	va_list ap2;
	va_copy(ap2, ap);
	int n = vformat(-1, (void *)0, 0, fmt, ap2);
	va_end(ap2);
	if (n < 0) {
		*strp = (void *)0;
		return -1;
	}
	char *buf = malloc((size_t)n + 1);
	if (!buf) {
		*strp = (void *)0;
		return -1;
	}
	vformat(-1, buf, (size_t)n + 1, fmt, ap);
	*strp = buf;
	return n;
}

int
asprintf(char **strp, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int r = vasprintf(strp, fmt, ap);
	va_end(ap);
	return r;
}

int
snprintf(char *str, size_t size, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	int r = vsnprintf(str, size, fmt, ap);
	va_end(ap);
	return r;
}

void
perror(const char *s)
{
	if (s && *s) {
		fputs(s, stderr);
		fputs(": ", stderr);
	}
	fputs(strerror(errno), stderr);
	fputc('\n', stderr);
}
