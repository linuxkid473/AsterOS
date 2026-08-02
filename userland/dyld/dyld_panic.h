/* Copyright (c) 2026 Vihaan Nathan
 *
 * dyld runs before any of the process's own error handling exists, so a
 * fatal load error just writes to stderr and exits -- no stdio formatting,
 * since we'd rather not depend on that machinery working correctly this
 * early.
 */
#ifndef DYLD_PANIC_H
#define DYLD_PANIC_H

void dyld_panic(const char *msg) __attribute__((noreturn));

#endif
