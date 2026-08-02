/*
 * /sbin/init replacement: mounts devfs, claims fd 0/1/2 on /dev/console
 * (same reasoning as the old userland/shell.c -- we are PID 1, exec'd
 * directly by the kernel with no inherited fd table), then execve()s
 * BusyBox as `busybox sh` so its own argv[1]-applet dispatch picks the
 * ash shell directly (avoids needing a /bin/sh symlink, which FAT16
 * doesn't support anyway).
 *
 * Built against userland/libc (the same shim BusyBox itself is built
 * against), not the old raw userland/syscall.h -- needs execve()/mount(),
 * which that header never had.
 */
#include <sys/mount.h>
#include <fcntl.h>
#include <unistd.h>

int
main(void)
{
	mount("devfs", "/dev", 0, 0);

	open("/dev/console", O_RDWR, 0);
	open("/dev/console", O_RDWR, 0);
	open("/dev/console", O_RDWR, 0);

	char *argv[] = { "/bin/busybox", "sh", (char *)0 };
	execve("/bin/busybox", argv, environ);

	/* execve only returns on failure -- with no shell at all, spin
	 * rather than let the kernel treat PID 1 exiting as fatal. */
	for (;;) {
	}
}
