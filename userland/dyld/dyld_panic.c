/* Copyright (c) 2026 Vihaan Nathan */
#include "dyld_panic.h"
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

void
dyld_panic(const char *msg)
{
	static const char prefix[] = "dyld: ";
	write(2, prefix, sizeof(prefix) - 1);
	write(2, msg, strlen(msg));
	write(2, "\n", 1);
	_exit(1);
}
