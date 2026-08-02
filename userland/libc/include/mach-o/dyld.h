/* No dynamic loader in this environment -- see dlfcn.h. This header exists
 * only so code that unconditionally #includes it on Darwin still compiles;
 * add real declarations here only once something actually calls into them. */
#ifndef _MACH_O_DYLD_H_
#define _MACH_O_DYLD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

struct mach_header;

/* Images are never unloaded here (nothing is ever dynamically loaded in
 * the first place), so registering for the "image removed" notification
 * is correctly a no-op -- see dl_stub.c. */
void _dyld_register_func_for_remove_image(
    void (*func)(const struct mach_header *mh, intptr_t vmaddr_slide));

/* No dyld to track the running image's real path, so this reports
 * argv[0] as captured at startup (see start.c) -- the same best-effort
 * fallback other minimal libcs use when there's no /proc/self/exe
 * either. Matches real _NSGetExecutablePath's own documented caveat
 * that it returns "a path", not necessarily a canonical real path. */
int _NSGetExecutablePath(char *buf, uint32_t *bufsize);

#ifdef __cplusplus
}
#endif

#endif /* _MACH_O_DYLD_H_ */
