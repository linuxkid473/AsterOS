/* Darwin's real uname(3) is implemented via sysctl(CTL_KERN, ...), not a
 * syscall -- rather than build out the sysctl MIB ABI just for this, we
 * hardcode the identity string this project already reports elsewhere
 * (userland/shell.c's cmd_uname), consistent with that existing choice. */
#include <sys/utsname.h>
#include <string.h>

int
uname(struct utsname *name)
{
	strlcpy(name->sysname, "Darwin", sizeof(name->sysname));
	strlcpy(name->nodename, "asteros", sizeof(name->nodename));
	strlcpy(name->release, "19.6.0", sizeof(name->release));
	strlcpy(name->version, "Darwin Kernel Version 19.6.0: xnu-6153.141.1 (AsterOS)", sizeof(name->version));
	strlcpy(name->machine, "x86_64", sizeof(name->machine));
	return 0;
}
