#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>

speed_t cfgetispeed(const struct termios *t) { return t->c_ispeed; }
speed_t cfgetospeed(const struct termios *t) { return t->c_ospeed; }
int cfsetispeed(struct termios *t, speed_t speed) { t->c_ispeed = speed; return 0; }
int cfsetospeed(struct termios *t, speed_t speed) { t->c_ospeed = speed; return 0; }
int
cfsetspeed(struct termios *t, speed_t speed)
{
	t->c_ispeed = speed;
	t->c_ospeed = speed;
	return 0;
}

void
cfmakeraw(struct termios *t)
{
	t->c_iflag &= ~(unsigned)(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	t->c_oflag &= ~(unsigned)OPOST;
	t->c_lflag &= ~(unsigned)(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	t->c_cflag &= ~(unsigned)(CSIZE | PARENB);
	t->c_cflag |= CS8;
	t->c_cc[VMIN] = 1;
	t->c_cc[VTIME] = 0;
}

int tcgetattr(int fd, struct termios *t) { return ioctl(fd, TIOCGETA, t); }

int
tcsetattr(int fd, int optional_actions, const struct termios *t)
{
	unsigned long req;
	switch (optional_actions) {
	case TCSADRAIN: req = TIOCSETAW; break;
	case TCSAFLUSH:  req = TIOCSETAF; break;
	default:         req = TIOCSETA; break;
	}
	return ioctl(fd, req, (void *)t);
}

int tcsendbreak(int fd, int len) { (void)fd; (void)len; return 0; }
int tcdrain(int fd) { (void)fd; return 0; }
int tcflush(int fd, int action) { (void)fd; (void)action; return 0; }
int tcflow(int fd, int action) { (void)fd; (void)action; return 0; }

pid_t
tcgetpgrp(int fd)
{
	int pgrp = 0;
	if (ioctl(fd, TIOCGPGRP, &pgrp) < 0) {
		return -1;
	}
	return pgrp;
}

int
tcsetpgrp(int fd, pid_t pgrp)
{
	int p = pgrp;
	return ioctl(fd, TIOCSPGRP, &p);
}
