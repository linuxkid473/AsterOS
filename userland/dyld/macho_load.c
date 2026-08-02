/* Copyright (c) 2026 Vihaan Nathan
 *
 * Mach-O segment mapping and load-command parsing, shared between the main
 * executable (already mapped by the kernel) and dylib dependencies (which
 * we load from disk ourselves -- the kernel only ever maps the main image
 * and dyld itself, per mach_loader.c's load_dylinker()).
 */
#include "image.h"
#include "dyld_panic.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>

struct image g_images[IMAGE_MAX];
int g_nimages;
struct image *g_main_image;

/* Dylibs get placed at fixed, well-spaced slots rather than through a real
 * VM address allocator -- there is no shared cache and no expectation of
 * more than a handful of dylibs, so "next free 256MB slot" is simpler than
 * writing an allocator we don't otherwise need. */
static uint64_t g_next_dylib_base = 0x0000000210000000ULL;
#define DYLIB_SLOT_SIZE 0x10000000ULL

void *
image_addr(const struct image *im, uint64_t unslid_vmaddr)
{
	return (void *)(uintptr_t)(unslid_vmaddr + im->slide);
}

const uint8_t *
image_file_offset_ptr(const struct image *im, uint32_t fileoff)
{
	for (int i = 0; i < im->nsegs; i++) {
		const struct segment *s = &im->segs[i];
		if (fileoff >= s->fileoff && (uint64_t)fileoff < s->fileoff + s->filesize) {
			return (const uint8_t *)image_addr(im, s->vmaddr + (fileoff - s->fileoff));
		}
	}
	return 0;
}

static void
translate_dyld_info(struct image *im, const struct dyld_info_command *dic)
{
	if (dic->rebase_size) {
		im->rebase_start = image_file_offset_ptr(im, dic->rebase_off);
		im->rebase_end = im->rebase_start + dic->rebase_size;
	}
	if (dic->bind_size) {
		im->bind_start = image_file_offset_ptr(im, dic->bind_off);
		im->bind_end = im->bind_start + dic->bind_size;
	}
	if (dic->weak_bind_size) {
		im->weak_bind_start = image_file_offset_ptr(im, dic->weak_bind_off);
		im->weak_bind_end = im->weak_bind_start + dic->weak_bind_size;
	}
	if (dic->lazy_bind_size) {
		im->lazy_bind_start = image_file_offset_ptr(im, dic->lazy_bind_off);
		im->lazy_bind_end = im->lazy_bind_start + dic->lazy_bind_size;
	}
	if (dic->export_size) {
		im->export_start = image_file_offset_ptr(im, dic->export_off);
		im->export_end = im->export_start + dic->export_size;
	}
}

struct image *
image_containing_address(uint64_t addr)
{
	for (int i = 0; i < g_nimages; i++) {
		struct image *im = &g_images[i];
		for (int j = 0; j < im->nsegs; j++) {
			uint64_t base = im->segs[j].vmaddr + im->slide;
			if (addr >= base && addr < base + im->segs[j].vmsize) {
				return im;
			}
		}
	}
	return 0;
}

static struct image *
find_loaded(const char *path)
{
	for (int i = 0; i < g_nimages; i++) {
		if (strcmp(g_images[i].path, path) == 0) {
			return &g_images[i];
		}
	}
	return 0;
}

/* Walks the load commands of an already-mapped image (either mmap'd by us
 * below, or by the kernel for the main executable) and fills in segs[],
 * the dyld-info pointers, and recursively loads LC_LOAD_DYLIB deps. */
void
image_parse_loaded(struct image *im, const struct mach_header_64 *mh, uint64_t slide)
{
	im->mh = mh;
	im->slide = slide;

	const uint8_t *cmd = (const uint8_t *)mh + sizeof(struct mach_header_64);
	for (uint32_t i = 0; i < mh->ncmds; i++) {
		const struct load_command *lc = (const struct load_command *)cmd;
		if (lc->cmd == LC_SEGMENT_64) {
			const struct segment_command_64 *sc = (const struct segment_command_64 *)lc;
			if (im->nsegs >= IMAGE_MAX_SEGMENTS) {
				dyld_panic("too many segments");
			}
			struct segment *s = &im->segs[im->nsegs++];
			memcpy(s->name, sc->segname, sizeof(s->name));
			s->vmaddr = sc->vmaddr;
			s->vmsize = sc->vmsize;
			s->fileoff = sc->fileoff;
			s->filesize = sc->filesize;
			/* The segment covering file offset 0 is the one the mach
			 * header actually sits in -- for an MH_EXECUTE with a
			 * __PAGEZERO guard segment (vmaddr 0, filesize 0), __PAGEZERO
			 * *also* reports fileoff 0, so filesize>0 is what actually
			 * distinguishes __TEXT here. */
			if (sc->fileoff == 0 && sc->filesize > 0) {
				im->preferred_base = sc->vmaddr;
			}
		} else if (lc->cmd == LC_DYLD_INFO_ONLY || lc->cmd == LC_DYLD_INFO) {
			translate_dyld_info(im, (const struct dyld_info_command *)lc);
		} else if (lc->cmd == LC_LOAD_DYLIB || lc->cmd == LC_LOAD_WEAK_DYLIB) {
			const struct dylib_command *dc = (const struct dylib_command *)lc;
			const char *deppath = (const char *)dc + dc->dylib.name.offset;
			if (im->ndeps >= IMAGE_MAX_DEPS) {
				dyld_panic("too many dependencies");
			}
			im->deps[im->ndeps++] = image_load_dependency(deppath);
		}
		cmd += lc->cmdsize;
	}
}

struct image *
image_load_dependency(const char *path)
{
	struct image *cached = find_loaded(path);
	if (cached) {
		return cached;
	}
	if (g_nimages >= IMAGE_MAX) {
		dyld_panic("too many loaded images");
	}
	struct image *im = &g_images[g_nimages++];
	memset(im, 0, sizeof(*im));
	strncpy(im->path, path, sizeof(im->path) - 1);

	int fd = open(path, O_RDONLY, 0);
	if (fd < 0) {
		dyld_panic(path);
	}

	struct mach_header_64 hdr;
	if (read(fd, &hdr, sizeof(hdr)) != (ssize_t)sizeof(hdr) || hdr.magic != MH_MAGIC_64) {
		dyld_panic("not a 64-bit Mach-O");
	}

	uint8_t *cmds = malloc(hdr.sizeofcmds);
	if (!cmds || read(fd, cmds, hdr.sizeofcmds) != (ssize_t)hdr.sizeofcmds) {
		dyld_panic("short read of load commands");
	}

	/* First pass: figure out the image's own preferred base and total
	 * span so we can pick one contiguous slot for every segment. */
	uint64_t lo = ~0ULL, hi = 0;
	uint8_t *cmd = cmds;
	for (uint32_t i = 0; i < hdr.ncmds; i++) {
		const struct load_command *lc = (const struct load_command *)cmd;
		if (lc->cmd == LC_SEGMENT_64) {
			const struct segment_command_64 *sc = (const struct segment_command_64 *)lc;
			if (sc->fileoff == 0 && sc->filesize > 0) {
				lo = sc->vmaddr;	/* segment holding the mach header */
			}
			if (sc->vmaddr + sc->vmsize > hi) {
				hi = sc->vmaddr + sc->vmsize;
			}
		}
		cmd += lc->cmdsize;
	}
	if (lo == ~0ULL || hi <= lo) {
		dyld_panic("no segments");
	}

	uint64_t span = hi - lo;
	uint64_t load_addr = g_next_dylib_base;
	uint64_t slots = (span + DYLIB_SLOT_SIZE - 1) / DYLIB_SLOT_SIZE;
	if (slots < 1) {
		slots = 1;
	}
	g_next_dylib_base += slots * DYLIB_SLOT_SIZE;
	uint64_t slide = load_addr - lo;

	/* Second pass: map each segment. We deliberately don't mmap the file
	 * directly (which would need fileoff page-alignment we can't always
	 * guarantee) -- anon-map the full vmsize (zeroed, covers zerofill
	 * tails like __DATA's __bss) then read() the file part on top. We
	 * also skip re-protecting segments after fixups (no mprotect wired
	 * up yet), so everything stays RWX -- a deliberate v1 simplification,
	 * not a correctness requirement of the format itself. */
	cmd = cmds;
	for (uint32_t i = 0; i < hdr.ncmds; i++) {
		const struct load_command *lc = (const struct load_command *)cmd;
		if (lc->cmd == LC_SEGMENT_64) {
			const struct segment_command_64 *sc = (const struct segment_command_64 *)lc;
			if (sc->vmsize > 0) {
				void *dest = (void *)(uintptr_t)(sc->vmaddr + slide);
				void *got = mmap(dest, sc->vmsize, PROT_READ | PROT_WRITE | PROT_EXEC,
				    MAP_FIXED | MAP_ANON | MAP_PRIVATE, -1, 0);
				if (got == MAP_FAILED || got != dest) {
					dyld_panic("segment mmap failed");
				}
				if (sc->filesize > 0) {
					lseek(fd, sc->fileoff, 0 /* SEEK_SET */);
					if (read(fd, dest, sc->filesize) != (ssize_t)sc->filesize) {
						dyld_panic("short read of segment contents");
					}
				}
			}
		}
		cmd += lc->cmdsize;
	}

	free(cmds);
	close(fd);

	/* The mach_header lives at the very start of __TEXT, i.e. at the
	 * segment covering file offset 0 -- which is exactly `lo`+slide. */
	image_parse_loaded(im, (const struct mach_header_64 *)(uintptr_t)(lo + slide), slide);
	return im;
}
