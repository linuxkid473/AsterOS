#ifndef _SCHED_H_
#define _SCHED_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Nothing else is ever runnable in this environment -- see pthread.h --
 * so yielding the (only) schedulable context is correctly a no-op. */
int sched_yield(void);

#ifdef __cplusplus
}
#endif

#endif /* _SCHED_H_ */
