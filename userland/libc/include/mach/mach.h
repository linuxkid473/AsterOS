/* No real userspace Mach IPC in this environment (only the BSD syscall
 * surface is implemented -- see docs/architecture.md). The one real
 * consumer of this header (llvm/lib/Support/Unix/Process.inc's
 * DisableSystemDialogsOnCrash) only uses it to ask the kernel to
 * silence macOS's CrashReporter UI -- a service that doesn't exist
 * here. task_get_exception_ports() honestly failing is the correct
 * outcome ("no such service to query"), not a shortcut: it makes the
 * caller's own error-handling skip the now-meaningless port-clearing
 * loop, exactly as if a real macOS process asked and got told no. */
#ifndef _MACH_MACH_H_
#define _MACH_MACH_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int mach_port_t;
typedef unsigned int mach_msg_type_number_t;
typedef unsigned int exception_mask_t;
typedef mach_port_t exception_port_t;
typedef int exception_behavior_t;
typedef int thread_state_flavor_t;
typedef int kern_return_t;

#define EXC_TYPES_COUNT 14
#define EXC_MASK_ALL    0xFFFFFFFFu
#define KERN_SUCCESS    0
#define KERN_FAILURE    5
#define MACH_PORT_NULL  ((mach_port_t)0)

mach_port_t mach_task_self(void);
kern_return_t task_get_exception_ports(mach_port_t task, exception_mask_t exception_mask,
    exception_mask_t *masks, mach_msg_type_number_t *count_out, exception_port_t *old_handlers,
    exception_behavior_t *old_behaviors, thread_state_flavor_t *old_flavors);
kern_return_t task_set_exception_ports(mach_port_t task, exception_mask_t exception_mask,
    exception_port_t new_port, exception_behavior_t new_behavior, thread_state_flavor_t new_flavor);

#ifdef __cplusplus
}
#endif

#endif /* _MACH_MACH_H_ */
