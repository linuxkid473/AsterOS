/* struct dirent layout matches getdirentries64's real on-wire record,
 * already ground-truthed in the existing userland/shell.c (struct
 * real_dirent64) -- reused here verbatim. */
#ifndef _DIRENT_H_
#define _DIRENT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

struct dirent {
	unsigned long long d_ino;
	unsigned long long d_seekoff;
	unsigned short      d_reclen;
	unsigned short      d_namlen;
	unsigned char       d_type;
	char                d_name[1024];
};

#define DT_UNKNOWN 0
#define DT_FIFO    1
#define DT_CHR     2
#define DT_DIR     4
#define DT_BLK     6
#define DT_REG     8
#define DT_LNK     10
#define DT_SOCK    12
#define DT_WHT     14

typedef struct {
	int fd;
	unsigned char buf[8192];
	size_t bufpos;
	size_t buflen;
	off_t  pos;
	struct dirent cur;
} DIR;

DIR *opendir(const char *path);
DIR *fdopendir(int fd);
struct dirent *readdir(DIR *dirp);
int closedir(DIR *dirp);
void rewinddir(DIR *dirp);
int dirfd(DIR *dirp);

#ifdef __cplusplus
}
#endif

#endif /* _DIRENT_H_ */
