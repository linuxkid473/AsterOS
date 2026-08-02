/* Console output/input helpers built on the raw read/write syscalls. */
#ifndef DARWINBUILD_CONSOLE_H
#define DARWINBUILD_CONSOLE_H

#include "syscall.h"
#include "mini_string.h"

static inline void
con_write(const char *s, size_t n)
{
	sys_write(1, s, n);
}

static inline void
con_puts(const char *s)
{
	con_write(s, xstrlen(s));
}

static inline void
con_putc(char c)
{
	sys_write(1, &c, 1);
}

static inline void
con_puthex64(unsigned long v)
{
	static const char digits[] = "0123456789abcdef";
	char buf[18];
	buf[0] = '0';
	buf[1] = 'x';
	for (int i = 0; i < 16; i++) {
		buf[2 + i] = digits[(v >> ((15 - i) * 4)) & 0xF];
	}
	con_write(buf, 18);
}

static inline void
con_putint(long v)
{
	char buf[24];
	int i = 0;
	int neg = 0;
	unsigned long uv;

	if (v < 0) {
		neg = 1;
		uv = (unsigned long)(-v);
	} else {
		uv = (unsigned long)v;
	}
	if (uv == 0) {
		con_putc('0');
		return;
	}
	while (uv) {
		buf[i++] = '0' + (uv % 10);
		uv /= 10;
	}
	if (neg) {
		con_putc('-');
	}
	while (i > 0) {
		con_putc(buf[--i]);
	}
}

/* Reads one line into buf (up to cap-1 bytes, always null-terminated,
 * trailing newline stripped). Relies on the tty already being in canonical
 * (cooked) mode -- xnu's BSD tty layer defaults to ICANON with echo on, so
 * the kernel itself handles backspace/line-editing before ever handing
 * bytes back to us; we just need to keep calling read() until we see '\n'
 * or fill the buffer. This is what makes PS/2 keyboard input "just work"
 * here without us writing any line-editing logic: the PS/2 driver feeds
 * raw keystrokes into the same tty input queue this read() drains. */
static inline int
con_readline(char *buf, size_t cap)
{
	size_t len = 0;
	while (len + 1 < cap) {
		char c;
		ssize_t n = sys_read(0, &c, 1);
		if (n <= 0) {
			/* EOF or error: return what we have so far, or -1 if
			 * nothing was read at all. */
			if (len == 0) {
				return -1;
			}
			break;
		}
		if (c == '\n' || c == '\r') {
			break;
		}
		buf[len++] = c;
	}
	buf[len] = 0;
	return (int)len;
}

#endif /* DARWINBUILD_CONSOLE_H */
