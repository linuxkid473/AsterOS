/* No real Mach IPC in this environment -- see mach/mach.h. ld64 (via
 * ld.cpp's getVMInfo()) calls mach_host_self()/host_statistics() purely
 * for optional `-stats` memory-pressure reporting and already handles
 * failure gracefully (bzero()s the result), so honest KERN_FAILURE
 * stubs (implemented in dl_stub.c) are correct here, matching
 * task_get/set_exception_ports in mach.h. */
#ifndef _MACH_MACH_HOST_H_
#define _MACH_MACH_HOST_H_

#include <mach/host_info.h>
#include <mach/kern_return.h>
#include <mach/mach_types.h>

#ifdef __cplusplus
extern "C" {
#endif

extern host_t mach_host_self(void);
extern kern_return_t host_statistics(host_t host_priv, host_flavor_t flavor,
    host_info_t host_info_out, mach_msg_type_number_t *host_info_outCnt);

#ifdef __cplusplus
}
#endif

#endif /* _MACH_MACH_HOST_H_ */
