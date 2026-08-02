/* The _IOx-style macros and TIOC* numbers below are ground-truthed against
 * src/xnu/bsd/sys/ioccom.h and src/xnu/bsd/sys/ttycom.h. */
#ifndef _SYS_IOCTL_H_
#define _SYS_IOCTL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

#define IOCPARM_MASK 0x1fff
#define IOC_VOID     0x20000000UL
#define IOC_OUT      0x40000000UL
#define IOC_IN       0x80000000UL
#define IOC_INOUT    (IOC_IN | IOC_OUT)

#define _IOC(inout, group, num, len) \
	((unsigned long)(inout | ((len & IOCPARM_MASK) << 16) | ((group) << 8) | (num)))
#define _IO(g, n)      _IOC(IOC_VOID, (g), (n), 0)
#define _IOR(g, n, t)  _IOC(IOC_OUT, (g), (n), sizeof(t))
#define _IOW(g, n, t)  _IOC(IOC_IN, (g), (n), sizeof(t))
#define _IOWR(g, n, t) _IOC(IOC_INOUT, (g), (n), sizeof(t))

struct winsize {
	unsigned short ws_row;
	unsigned short ws_col;
	unsigned short ws_xpixel;
	unsigned short ws_ypixel;
};

struct termios; /* see termios.h */

#define TIOCGETA   _IOR('t', 19, struct termios)
#define TIOCSETA   _IOW('t', 20, struct termios)
#define TIOCSETAW  _IOW('t', 21, struct termios)
#define TIOCSETAF  _IOW('t', 22, struct termios)
#define TIOCGWINSZ _IOR('t', 104, struct winsize)
#define TIOCSWINSZ _IOW('t', 103, struct winsize)
#define TIOCGPGRP  _IOR('t', 119, int)
#define TIOCSPGRP  _IOW('t', 118, int)
#define TIOCSCTTY  _IO('t', 97)
#define FIONREAD   _IOR('f', 127, int)

int ioctl(int fd, unsigned long request, ...);

#ifdef __cplusplus
}
#endif

#endif /* _SYS_IOCTL_H_ */
