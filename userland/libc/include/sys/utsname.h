#ifndef _SYS_UTSNAME_H_
#define _SYS_UTSNAME_H_

#ifdef __cplusplus
extern "C" {
#endif

#define _SYS_NAMELEN 256

struct utsname {
	char sysname[_SYS_NAMELEN];
	char nodename[_SYS_NAMELEN];
	char release[_SYS_NAMELEN];
	char version[_SYS_NAMELEN];
	char machine[_SYS_NAMELEN];
};

int uname(struct utsname *name);

#ifdef __cplusplus
}
#endif

#endif /* _SYS_UTSNAME_H_ */
