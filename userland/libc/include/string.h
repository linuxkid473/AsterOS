#ifndef _STRING_H_
#define _STRING_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

void  *memcpy(void *dst, const void *src, size_t n);
void  *mempcpy(void *dst, const void *src, size_t n);
void  *memmove(void *dst, const void *src, size_t n);
void  *memset(void *dst, int c, size_t n);
void   memset_pattern16(void *dst, const void *pattern16, size_t n);
void   bzero(void *dst, size_t n);
int    memcmp(const void *a, const void *b, size_t n);
void  *memchr(const void *s, int c, size_t n);

size_t strlen(const char *s);
size_t strnlen(const char *s, size_t maxlen);
char  *strcpy(char *dst, const char *src);
char  *stpcpy(char *dst, const char *src);
char  *stpncpy(char *dst, const char *src, size_t n);
char  *strncpy(char *dst, const char *src, size_t n);
char  *strcat(char *dst, const char *src);
char  *strncat(char *dst, const char *src, size_t n);
int    strcmp(const char *a, const char *b);
int    strncmp(const char *a, const char *b, size_t n);
int    strcasecmp(const char *a, const char *b);
int    strncasecmp(const char *a, const char *b, size_t n);
char  *strchr(const char *s, int c);
char  *strrchr(const char *s, int c);
char  *strstr(const char *hay, const char *needle);
char  *strdup(const char *s);
size_t strxfrm(char *dst, const char *src, size_t n);
int    strcoll(const char *s1, const char *s2);
char  *strndup(const char *s, size_t n);
size_t strspn(const char *s, const char *accept);
size_t strcspn(const char *s, const char *reject);
char  *strpbrk(const char *s, const char *accept);
char  *strtok(char *s, const char *delim);
char  *strtok_r(char *s, const char *delim, char **saveptr);
char  *strsep(char **stringp, const char *delim);
size_t strlcpy(char *dst, const char *src, size_t size);
size_t strlcat(char *dst, const char *src, size_t size);
char  *strerror(int errnum);
int    strerror_r(int errnum, char *buf, size_t buflen);
char  *strsignal(int sig);

#ifdef __cplusplus
}
#endif

#endif /* _STRING_H_ */
