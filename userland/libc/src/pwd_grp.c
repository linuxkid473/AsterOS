/* No /etc/passwd or /etc/group parsing -- a single synthetic root
 * identity, which is all this environment ever runs as (see pwd.h/grp.h
 * for why). */
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "syscall_raw.h"

static struct passwd g_root_pw = {
	.pw_name = "root",
	.pw_passwd = "*",
	.pw_uid = 0,
	.pw_gid = 0,
	.pw_gecos = "root",
	.pw_dir = "/",
	.pw_shell = "/bin/sh",
};

static char *g_root_grp_members[] = { "root", (void *)0 };
static struct group g_root_gr = {
	.gr_name = "root",
	.gr_passwd = "*",
	.gr_gid = 0,
	.gr_mem = g_root_grp_members,
};

struct passwd *
getpwnam(const char *name)
{
	if (strcmp(name, "root") == 0) {
		return &g_root_pw;
	}
	return (void *)0;
}

struct passwd *
getpwuid(uid_t uid)
{
	if (uid == 0) {
		return &g_root_pw;
	}
	return (void *)0;
}

/* buf/bufsize are unused: g_root_pw's fields point at static strings
 * that outlive every caller, and there is only ever the one root user,
 * so there is no real reentrancy hazard here to guard against (see the
 * file-level note). */
int
getpwnam_r(const char *name, struct passwd *pwd, char *buf, size_t bufsize, struct passwd **result)
{
	(void)buf; (void)bufsize;
	struct passwd *p = getpwnam(name);
	if (!p) {
		*result = (void *)0;
		return 0;
	}
	*pwd = *p;
	*result = pwd;
	return 0;
}

int
getpwuid_r(uid_t uid, struct passwd *pwd, char *buf, size_t bufsize, struct passwd **result)
{
	(void)buf; (void)bufsize;
	struct passwd *p = getpwuid(uid);
	if (!p) {
		*result = (void *)0;
		return 0;
	}
	*pwd = *p;
	*result = pwd;
	return 0;
}

struct passwd *getpwent(void) { return (void *)0; }
void setpwent(void) {}
void endpwent(void) {}

struct group *
getgrnam(const char *name)
{
	if (strcmp(name, "root") == 0) {
		return &g_root_gr;
	}
	return (void *)0;
}

struct group *
getgrgid(gid_t gid)
{
	if (gid == 0) {
		return &g_root_gr;
	}
	return (void *)0;
}

struct group *getgrent(void) { return (void *)0; }
void setgrent(void) {}
void endgrent(void) {}

int
setgroups(int ngroups, const gid_t grouplist[])
{
	return (int)sys_result(raw_syscall2(80 /* SYS_setgroups */, ngroups, (long)grouplist));
}

int
initgroups(const char *name, gid_t basegid)
{
	(void)name;
	gid_t g = basegid;
	return setgroups(1, &g);
}
