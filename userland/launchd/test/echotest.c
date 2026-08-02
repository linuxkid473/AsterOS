/* Copyright (c) 2026 Vihaan Nathan
 *
 * Minimal KeepAlive regression daemon: appends one line and exits
 * immediately, so a live QEMU run can confirm launchd's supervision loop
 * actually re-forks on exit (not just once at boot) by watching the line
 * count in /var/log/echotest.log grow over time.
 */
#include <fcntl.h>
#include <unistd.h>

int
main(void)
{
	int fd = open("/var/log/echotest.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
	if (fd >= 0) {
		write(fd, "tick\n", 5);
		close(fd);
	}
	return 0;
}
