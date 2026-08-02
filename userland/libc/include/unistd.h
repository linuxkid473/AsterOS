#ifndef _UNISTD_H_
#define _UNISTD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <sys/_types/_uuid_t.h> /* matches real Darwin's unistd.h -> gethostuuid.h chain */

struct timespec;
int gethostuuid(uuid_t out, const struct timespec *wait);

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define STDIN_FILENO  0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#define F_OK 0
#define X_OK 1
#define W_OK 2
#define R_OK 4

extern char **environ;

ssize_t read(int fd, void *buf, size_t nbyte);
ssize_t write(int fd, const void *buf, size_t nbyte);
ssize_t pread(int fd, void *buf, size_t nbyte, off_t offset);
ssize_t pwrite(int fd, const void *buf, size_t nbyte, off_t offset);
int mkpath_np(const char *path, mode_t omode);
int close(int fd);
int dup(int fd);
int dup2(int fd, int fd2);
int pipe(int fildes[2]);
off_t lseek(int fd, off_t offset, int whence);
int ftruncate(int fd, off_t length);
int truncate(const char *path, off_t length);
int unlink(const char *path);
int unlinkat(int fd, const char *path, int flag);
int rmdir(const char *path);
int chdir(const char *path);
int fchdir(int fd);
int chroot(const char *path);
int setuid(uid_t uid);
int setgid(gid_t gid);
int seteuid(uid_t euid);
int setegid(gid_t egid);
int ttyname_r(int fd, char *buf, size_t buflen);
char *ttyname(int fd);
char *getcwd(char *buf, size_t size);
int access(const char *path, int amode);
int chown(const char *path, uid_t owner, gid_t group);
int fchown(int fd, uid_t owner, gid_t group);
int lchown(const char *path, uid_t owner, gid_t group);
int symlink(const char *target, const char *linkpath);
ssize_t readlink(const char *path, char *buf, size_t bufsize);
int link(const char *path1, const char *path2);
int isatty(int fd);
pid_t getpid(void);
pid_t getppid(void);
uid_t getuid(void);
uid_t geteuid(void);
gid_t getgid(void);
gid_t getegid(void);
int getgroups(int gidsetsize, gid_t grouplist[]);

#define _SC_CLK_TCK   3
#define _SC_PAGESIZE  29
#define _SC_PAGE_SIZE _SC_PAGESIZE
#define _SC_ARG_MAX   1
#define _SC_OPEN_MAX  5
#define _SC_NPROCESSORS_ONLN 58
#define _SC_NPROCESSORS_CONF 57
#define _SC_GETPW_R_SIZE_MAX 71
long sysconf(int name);
pid_t fork(void);
pid_t vfork(void);
int execve(const char *path, char *const argv[], char *const envp[]);
int execv(const char *path, char *const argv[]);
int execvp(const char *file, char *const argv[]);
void _exit(int status) __attribute__((noreturn));
unsigned int sleep(unsigned int seconds);
unsigned int alarm(unsigned int seconds);
int usleep(unsigned int usecs);
pid_t setsid(void);
pid_t getpgrp(void);
pid_t getsid(pid_t pid);
int setpgid(pid_t pid, pid_t pgid);
char *getlogin(void);

extern char *optarg;
extern int optind, opterr, optopt;
int getopt(int argc, char *const argv[], const char *optstring);

#ifdef __cplusplus
}
#endif

#endif /* _UNISTD_H_ */
