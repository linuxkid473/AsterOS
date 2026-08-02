/* Minimal owned FILE* implementation -- not Apple's struct __sFILE ABI
 * (that's a complex opaque-cookie design tied to Libsystem internals we
 * don't have). Self-contained: fully implemented in libc/src/stdio.c,
 * nothing here depends on dyld/Libsystem. */
#ifndef _STDIO_H_
#define _STDIO_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <stdarg.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

#define EOF (-1)
#define BUFSIZ 1024
#define FOPEN_MAX 64
#define FILENAME_MAX 1024
#define L_tmpnam 64

#define _IOFBF 0
#define _IOLBF 1
#define _IONBF 2

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

typedef struct FILE {
	int   fd;
	int   flags;      /* internal: EOF/error/buffering mode seen */
	int   eof;
	int   error;
	unsigned char *buf;
	size_t bufsize;
	size_t rpos, rlen; /* read buffer window */
	int   ungot;       /* pushback byte, or -1 */
} FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;
#define stdin  stdin
#define stdout stdout
#define stderr stderr

FILE *fopen(const char *path, const char *mode);
FILE *fdopen(int fd, const char *mode);
FILE *freopen(const char *path, const char *mode, FILE *stream);
int   fclose(FILE *stream);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
int   fflush(FILE *stream);
void  setbuf(FILE *stream, char *buf);
int   setvbuf(FILE *stream, char *buf, int mode, size_t size);
int   fseek(FILE *stream, long offset, int whence);
long  ftell(FILE *stream);
int   fseeko(FILE *stream, off_t offset, int whence);
off_t ftello(FILE *stream);
void  rewind(FILE *stream);
int   feof(FILE *stream);
int   ferror(FILE *stream);
void  clearerr(FILE *stream);
int   fileno(FILE *stream);

int   fgetc(FILE *stream);
int   getc(FILE *stream);
int   getchar(void);
char *fgets(char *s, int size, FILE *stream);
int   ungetc(int c, FILE *stream);

int   fputc(int c, FILE *stream);
int   putc(int c, FILE *stream);
int   putchar(int c);
int   fputs(const char *s, FILE *stream);
int   puts(const char *s);

int   printf(const char *fmt, ...);
int   fprintf(FILE *stream, const char *fmt, ...);
int   sprintf(char *str, const char *fmt, ...);
int   snprintf(char *str, size_t size, const char *fmt, ...);
int   vprintf(const char *fmt, va_list ap);
int   vfprintf(FILE *stream, const char *fmt, va_list ap);
int   vsprintf(char *str, const char *fmt, va_list ap);
int   vsnprintf(char *str, size_t size, const char *fmt, va_list ap);
int   asprintf(char **strp, const char *fmt, ...);
int   vasprintf(char **strp, const char *fmt, va_list ap);

int   sscanf(const char *str, const char *fmt, ...);
int   vsscanf(const char *str, const char *fmt, va_list ap);
int   fscanf(FILE *stream, const char *fmt, ...);

void  perror(const char *s);
int   remove(const char *path);
int   rename(const char *old, const char *new_);

#ifdef __cplusplus
}
#endif

#endif /* _STDIO_H_ */
