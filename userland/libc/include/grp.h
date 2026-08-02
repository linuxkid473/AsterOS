#ifndef _GRP_H_
#define _GRP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

struct group {
	char  *gr_name;
	char  *gr_passwd;
	gid_t  gr_gid;
	char **gr_mem;
};

int setgroups(int ngroups, const gid_t grouplist[]);
int initgroups(const char *name, gid_t basegid);
struct group *getgrnam(const char *name);
struct group *getgrgid(gid_t gid);
struct group *getgrent(void);
void setgrent(void);
void endgrent(void);

#ifdef __cplusplus
}
#endif

#endif /* _GRP_H_ */
