/* No dynamic loader in this environment (no dyld -- see
 * docs/architecture.md): every executable here is a static, dyld-free
 * Mach-O with no LC_LOAD_DYLINKER, so there is nothing dlopen() could
 * ever open. Declaring the real POSIX/Darwin surface and having it
 * honestly report "not available" is correct behavior for a system with
 * no shared libraries, not a stub pretending to work -- see dl_stub.c. */
#ifndef _DLFCN_H_
#define _DLFCN_H_

#ifdef __cplusplus
extern "C" {
#endif

#define RTLD_LAZY     0x1
#define RTLD_NOW      0x2
#define RTLD_LOCAL    0x4
#define RTLD_GLOBAL   0x8
#define RTLD_NOLOAD   0x10
#define RTLD_NODELETE 0x80
#define RTLD_FIRST    0x100

#define RTLD_DEFAULT   ((void *)-2)
#define RTLD_SELF      ((void *)-3)
#define RTLD_MAIN_ONLY ((void *)-5)

void *dlopen(const char *path, int mode);
int   dlclose(void *handle);
void *dlsym(void *handle, const char *symbol);
char *dlerror(void);

typedef struct {
	const char *dli_fname;
	void       *dli_fbase;
	const char *dli_sname;
	void       *dli_saddr;
} Dl_info;

int dladdr(const void *addr, Dl_info *info);

#ifdef __cplusplus
}
#endif

#endif /* _DLFCN_H_ */
