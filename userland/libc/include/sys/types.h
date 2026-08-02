/* Minimal sys/types.h for our no-dyld Darwin libc shim.
 * Field widths ground-truthed against src/xnu/bsd/sys headers for xnu-6153
 * (x86_64, __DARWIN_64_BIT_INO_T==1, __DARWIN_UNIX03==1 -- see
 * docs/architecture.md / patches for how that was confirmed) -- NOT the
 * generic POSIX text, since what matters here is matching this exact
 * kernel's syscall ABI.
 */
#ifndef _SYS_TYPES_H_
#define _SYS_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

typedef long ssize_t;
typedef long off_t;
typedef int pid_t;
typedef unsigned short mode_t;
typedef unsigned short nlink_t;
typedef unsigned long long ino_t;
typedef unsigned long long ino64_t;
typedef int dev_t;
typedef unsigned int uid_t;
typedef unsigned int gid_t;
typedef long blkcnt_t;
typedef int blksize_t;
typedef unsigned int fflags_t;
typedef long time_t;
typedef long suseconds_t;
typedef unsigned int useconds_t;
typedef long clock_t;
typedef unsigned long tcflag_t;
typedef unsigned char cc_t;
typedef unsigned long speed_t;
typedef unsigned int sigset_t;
typedef unsigned long u_long;
typedef unsigned int u_int;
typedef unsigned short u_short;
typedef unsigned char u_char;
typedef long key_t;
typedef char *caddr_t;

#ifdef __cplusplus
}
#endif

#endif /* _SYS_TYPES_H_ */
