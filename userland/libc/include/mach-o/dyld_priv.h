/* Minimal stand-in for Apple's private mach-o/dyld_priv.h.
 * ld64's src/ld/parsers/libunwind/AddressSpace.hpp only needs the
 * dyld_unwind_sections type name and this declaration to type-check its
 * LocalAddressSpace::findUnwindSections() method -- that method (for
 * introspecting the CURRENTLY RUNNING process' own unwind info) is never
 * actually called by ld64 itself (it only ever unwinds OTHER files' data
 * via ObjectAddressSpace), so _dyld_find_unwind_sections never needs a
 * real definition, only a declaration to satisfy the compiler.
 */
#ifndef _MACH_O_DYLD_PRIV_H_
#define _MACH_O_DYLD_PRIV_H_

#include <mach-o/loader.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct dyld_unwind_sections {
	const struct mach_header *mh;
	const void               *dwarf_section;
	uintptr_t                 dwarf_section_length;
	const void                *compact_unwind_section;
	uintptr_t                 compact_unwind_section_length;
};

extern bool _dyld_find_unwind_sections(void *addr, struct dyld_unwind_sections *info);

#ifdef __cplusplus
}
#endif

#endif /* _MACH_O_DYLD_PRIV_H_ */
