/* No dynamic loader in this environment -- see dlfcn.h. Every one of these
 * honestly reports "nothing to find" rather than pretending to succeed. */
#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <mach-o/dyld_priv.h>
#include <mach-o/loader.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>

extern int errno;

void *dlopen(const char *path, int mode) { (void)path; (void)mode; errno = ENOSYS; return NULL; }
int   dlclose(void *handle) { (void)handle; errno = ENOSYS; return -1; }
void *dlsym(void *handle, const char *symbol) { (void)handle; (void)symbol; errno = ENOSYS; return NULL; }
char *dlerror(void) { return "dynamic loading is not supported on this system"; }
int   dladdr(const void *addr, Dl_info *info) { (void)addr; (void)info; return 0; /* no symbol info available */ }

/* x86_64 has coherent instruction/data caches, so there is nothing to
 * flush -- a no-op here is correct hardware behavior, not a shortcut
 * (real Darwin's own x86_64 libSystem implementation is a no-op too;
 * only the ARM/PowerPC variants of this function do real cache-line
 * flush instructions). */
void sys_icache_invalidate(const void *addr, size_t len) { (void)addr; (void)len; }

#include <mach/mach.h>
#include <mach/mach_host.h>
mach_port_t mach_task_self(void) { return 1; }

host_t
mach_host_self(void)
{
	return 1;
}

kern_return_t
host_statistics(host_t host_priv, host_flavor_t flavor, host_info_t host_info_out,
    mach_msg_type_number_t *host_info_outCnt)
{
	(void)host_priv; (void)flavor; (void)host_info_out; (void)host_info_outCnt;
	return KERN_FAILURE; /* no real Mach IPC in this environment -- see mach_host.h */
}

#include <sys/sysctl.h>
#include <string.h>

int
sysctl(int *name, unsigned int namelen, void *oldp, size_t *oldlenp, void *newp, size_t newlen)
{
	(void)newp; (void)newlen;
	if (namelen == 2 && name[0] == CTL_KERN && name[1] == KERN_OSRELEASE) {
		static const char release[] = "19.6.0"; /* matches uname(3)'s release -- see uname.c */
		if (oldp && oldlenp) {
			size_t n = sizeof(release) < *oldlenp ? sizeof(release) : *oldlenp;
			memcpy(oldp, release, n);
			*oldlenp = sizeof(release);
		}
		return 0;
	}
	if (namelen == 2 && name[0] == CTL_HW && name[1] == HW_NCPU) {
		/* Honestly 1: pthread_create() always returns EAGAIN here (see
		 * pthread_stub.c), so no extra worker thread ld64 might spawn
		 * based on this count could ever actually run. */
		static const int ncpu = 1;
		if (oldp && oldlenp) {
			size_t n = sizeof(ncpu) < *oldlenp ? sizeof(ncpu) : *oldlenp;
			memcpy(oldp, &ncpu, n);
			*oldlenp = sizeof(ncpu);
		}
		return 0;
	}
	errno = ENOTSUP;
	return -1;
}

int
sysctlbyname(const char *name, void *oldp, size_t *oldlenp, void *newp, size_t newlen)
{
	(void)name; (void)oldp; (void)oldlenp; (void)newp; (void)newlen;
	errno = ENOTSUP;
	return -1;
}
kern_return_t
task_get_exception_ports(mach_port_t task, exception_mask_t exception_mask,
    exception_mask_t *masks, mach_msg_type_number_t *count_out, exception_port_t *old_handlers,
    exception_behavior_t *old_behaviors, thread_state_flavor_t *old_flavors)
{
	(void)task; (void)exception_mask; (void)masks; (void)old_handlers;
	(void)old_behaviors; (void)old_flavors;
	*count_out = 0;
	return KERN_FAILURE; /* no such service in this environment -- see mach.h */
}
kern_return_t
task_set_exception_ports(mach_port_t task, exception_mask_t exception_mask,
    exception_port_t new_port, exception_behavior_t new_behavior, thread_state_flavor_t new_flavor)
{
	(void)task; (void)exception_mask; (void)new_port; (void)new_behavior; (void)new_flavor;
	return KERN_FAILURE;
}

/* Not actually reached by anything we build (HAVE_MALLOC_ZONE_STATISTICS
 * is off, see llvm/lib/Support/Unix/Process.inc's GetMallocUsage) --
 * declared/defined only so the header's mere presence doesn't leave a
 * dangling declaration if that ever changes. */
#include <malloc/malloc.h>
malloc_zone_t *malloc_default_zone(void) { return (malloc_zone_t *)0; }
void malloc_zone_statistics(malloc_zone_t *zone, malloc_statistics_t *stats) { (void)zone; stats->blocks_in_use = 0; stats->size_in_use = 0; stats->max_size_in_use = 0; stats->size_allocated = 0; }
size_t malloc_size(const void *ptr) { (void)ptr; return 0; }

#include <execinfo.h>
int backtrace(void **buffer, int size) { (void)buffer; (void)size; return 0; }
char **backtrace_symbols(void *const *buffer, int size) { (void)buffer; (void)size; return (char **)0; }
void backtrace_symbols_fd(void *const *buffer, int size, int fd) { (void)buffer; (void)size; (void)fd; }

#include <mach-o/dyld.h>
#include <string.h>
extern const char *__libc_argv0;
int
_NSGetExecutablePath(char *buf, uint32_t *bufsize)
{
	size_t need = strlen(__libc_argv0) + 1;
	if (*bufsize < need) {
		*bufsize = (uint32_t)need;
		return -1;
	}
	memcpy(buf, __libc_argv0, need);
	return 0;
}

void
_dyld_register_func_for_remove_image(
    void (*func)(const struct mach_header *mh, intptr_t vmaddr_slide))
{
	(void)func; /* nothing is ever dynamically loaded, so images are
	             * never removed either -- callback correctly never fires */
}
