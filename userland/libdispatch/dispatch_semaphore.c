/* Copyright (c) 2026 Vihaan Nathan
 *
 * dispatch_semaphore_t on real pthread_mutex_t/pthread_cond_t -- see
 * dispatch.h's own header comment for why not mach semaphore_create/
 * semaphore_wait (no syscall wrapper for those Mach traps in this tree).
 */
#include "dispatch_internal.h"
#include <stdlib.h>
#include <errno.h>

static void
_dispatch_semaphore_destroy(void *self)
{
	dispatch_semaphore_t sem = self;
	pthread_mutex_destroy(&sem->lock);
	pthread_cond_destroy(&sem->cond);
	free(sem);
}

dispatch_semaphore_t
dispatch_semaphore_create(long value)
{
	dispatch_semaphore_t sem = malloc(sizeof(*sem));
	_dispatch_object_init(&sem->hdr, _dispatch_semaphore_destroy);
	pthread_mutex_init(&sem->lock, NULL);
	pthread_cond_init(&sem->cond, NULL);
	sem->value = value;
	return sem;
}

long
dispatch_semaphore_signal(dispatch_semaphore_t sem)
{
	pthread_mutex_lock(&sem->lock);
	long value = ++sem->value;
	if (value > 0) {
		pthread_cond_signal(&sem->cond);
	}
	pthread_mutex_unlock(&sem->lock);
	return value > 0 ? 0 : 1;
}

long
dispatch_semaphore_wait(dispatch_semaphore_t sem, dispatch_time_t timeout)
{
	pthread_mutex_lock(&sem->lock);
	long result = 0;
	if (timeout == DISPATCH_TIME_FOREVER) {
		while (sem->value <= 0) {
			pthread_cond_wait(&sem->cond, &sem->lock);
		}
		sem->value--;
	} else if (timeout == DISPATCH_TIME_NOW) {
		if (sem->value > 0) {
			sem->value--;
		} else {
			result = -1;
		}
	} else {
		struct timespec ts = { (time_t)(timeout / 1000000000ull), (long)(timeout % 1000000000ull) };
		while (sem->value <= 0) {
			if (pthread_cond_timedwait(&sem->cond, &sem->lock, &ts) == ETIMEDOUT) {
				if (sem->value <= 0) {
					result = -1;
					break;
				}
			}
		}
		if (result == 0) {
			sem->value--;
		}
	}
	pthread_mutex_unlock(&sem->lock);
	return result;
}
