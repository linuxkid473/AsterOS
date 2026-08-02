/* No /etc/passwd parsing -- a single synthetic root user, real enough for
 * ash's prompt/~-expansion use and for `id`-style output. */
#ifndef _PWD_H_
#define _PWD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

struct passwd {
	char  *pw_name;
	char  *pw_passwd;
	uid_t  pw_uid;
	gid_t  pw_gid;
	char  *pw_gecos;
	char  *pw_dir;
	char  *pw_shell;
};

struct passwd *getpwnam(const char *name);
struct passwd *getpwuid(uid_t uid);
struct passwd *getpwent(void);
void setpwent(void);
void endpwent(void);

int getpwnam_r(const char *name, struct passwd *pwd, char *buf, size_t bufsize, struct passwd **result);
int getpwuid_r(uid_t uid, struct passwd *pwd, char *buf, size_t bufsize, struct passwd **result);

#ifdef __cplusplus
}
#endif

#endif /* _PWD_H_ */
