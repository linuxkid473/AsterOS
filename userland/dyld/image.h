/* Copyright (c) 2026 Vihaan Nathan
 *
 * An `image` is our in-memory record of one loaded Mach-O (the main
 * executable or a dylib dependency) -- everything the rebase/bind/export
 * passes need, gathered once at load time instead of re-walking load
 * commands from scratch every lookup.
 */
#ifndef DYLD_IMAGE_H
#define DYLD_IMAGE_H

#include <stdint.h>
#include <mach-o/loader.h>

#define IMAGE_MAX 16
#define IMAGE_MAX_SEGMENTS 12
#define IMAGE_MAX_DEPS 8

struct segment {
	char name[16];
	uint64_t vmaddr;	/* as linked, unslid */
	uint64_t vmsize;
	uint64_t fileoff;
	uint64_t filesize;
};

struct image {
	char path[256];
	int fd;

	const struct mach_header_64 *mh;	/* slid, mapped address */
	uint64_t slide;
	uint64_t preferred_base;	/* lowest segment vmaddr, unslid */

	struct segment segs[IMAGE_MAX_SEGMENTS];
	int nsegs;

	/* LC_DYLD_INFO_ONLY byte ranges, translated to slid addresses in our
	 * own address space (we mmap'd the segment containing LINKEDIT, so
	 * these are ordinary readable pointers, not file offsets). */
	const uint8_t *rebase_start, *rebase_end;
	const uint8_t *bind_start, *bind_end;
	const uint8_t *weak_bind_start, *weak_bind_end;
	const uint8_t *lazy_bind_start, *lazy_bind_end;
	const uint8_t *export_start, *export_end;

	/* dylib ordinals in a bind opcode stream are 1-based indices into
	 * this image's own LC_LOAD_DYLIB list, in load-command order --
	 * ground-truthed against ld64's bind opcode emission. */
	struct image *deps[IMAGE_MAX_DEPS];
	int ndeps;
};

/* Hidden visibility matters for correctness here, not just style: an
 * extern declaration with no explicit visibility gets compiled by
 * whoever *references* it (not whoever defines it) as potentially
 * interposable under -fPIC, which routes access through a GOT slot that
 * only a rebase pass would populate -- and dyld never rebases its own
 * image. -fvisibility=hidden in build.sh covers symbols each TU defines;
 * these cross-TU externs need it spelled out explicitly too. */
#define DYLD_HIDDEN __attribute__((visibility("hidden")))

extern struct image g_images[IMAGE_MAX] DYLD_HIDDEN;
extern int g_nimages DYLD_HIDDEN;
extern struct image *g_main_image DYLD_HIDDEN;

void *image_addr(const struct image *im, uint64_t unslid_vmaddr);
const uint8_t *image_file_offset_ptr(const struct image *im, uint32_t fileoff);

/* Loads a dependency dylib from disk: opens path, chooses a load address,
 * mmaps its segments, and parses its load commands (recursing into its
 * own LC_LOAD_DYLIB deps). Returns the (possibly cached) image. */
struct image *image_load_dependency(const char *path);

/* Main-executable variant: the kernel already mapped it, we just need to
 * walk its load commands in place. */
void image_parse_loaded(struct image *im, const struct mach_header_64 *mh, uint64_t slide);

uint64_t image_resolve_export(struct image *im, const char *symbol);
void image_rebase(struct image *im);
void image_bind(struct image *im);
uint64_t dyld_bind_lazy_symbol(struct image *im, uint32_t lazy_offset);
struct image *image_containing_address(uint64_t addr);
void image_run_mod_init_funcs(struct image *im);

#endif /* DYLD_IMAGE_H */
