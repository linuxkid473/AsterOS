/* Copyright (c) 2026 Vihaan Nathan
 *
 * _objc_init: the one exported symbol userland/dyld/dyld_main.c looks
 * up by name and calls once, after every loaded image is rebased/bound
 * but before any image's __DATA,__mod_init_func constructors run --
 * this project's hardcoded stand-in for real dyld's generic
 * _dyld_objc_notify_register callback (see TODO.md Phase 13).
 */
#include "objc_priv.h"

void
_objc_init(const struct mach_header_64 *const *mhs, int count)
{
	objc_selector_init();
	for (int i = 0; i < count; i++) {
		if (mhs[i]) {
			objc_register_image(mhs[i]);
		}
	}
	objc_attach_categories();
}
