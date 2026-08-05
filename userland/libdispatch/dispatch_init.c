/* Copyright (c) 2026 Vihaan Nathan
 *
 * Starts the worker pool + timer thread once per process, via the same
 * __attribute__((constructor)) mechanism userland/Foundation/
 * FoundationInit.m uses -- runs before main(), so dispatch_get_main_queue()/
 * dispatch_get_global_queue() are always safe to call from the very first
 * line of a client's main().
 */
#include "dispatch_internal.h"

__attribute__((constructor))
static void
__DispatchInit(void)
{
	_dispatch_workerpool_start();
	_dispatch_timer_thread_start();
}
