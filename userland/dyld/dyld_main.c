/* Copyright (c) 2026 Vihaan Nathan
 *
 * Top-level bootstrap, called from dyld_start.S once the main executable's
 * mach_header is off the stack. Loads every LC_LOAD_DYLIB dependency,
 * rebases and binds everything, then hands back the main executable's
 * real entry point for dyld_start.S to jump to.
 */
#include "image.h"
#include "dyld_panic.h"
#include <mach-o/loader.h>
#include <mach-o/dyld_images.h>
#include <string.h>

/* Named so mach_loader.c's note_all_image_info_section() (bsd/kern/
 * mach_loader.c) can find and report it via task_info() for debuggers --
 * xnu only locates this region by section name, it never interprets the
 * contents, so we're free to leave most fields at their version-1 zero
 * defaults. */
__attribute__((section("__DATA,__all_image_info"), used))
static struct dyld_all_image_infos g_all_image_infos;

static struct dyld_image_info g_image_info_array[IMAGE_MAX];

/* ld64 synthesizes this for every MH_DYLINKER/MH_EXECUTE/MH_DYLIB output
 * (InputFiles.cpp's DSOHandleAtom) -- our own mach_header address, without
 * needing to thread it through from dyld_start.S. See DYLD_HIDDEN in
 * image.h for why the explicit visibility attribute is load-bearing. */
extern struct mach_header_64 _mh_dylinker_header DYLD_HIDDEN;

static uint64_t
find_preferred_base(const struct mach_header_64 *mh)
{
	const uint8_t *cmd = (const uint8_t *)mh + sizeof(*mh);
	uint64_t found = 0;
	int have = 0;
	for (uint32_t i = 0; i < mh->ncmds; i++) {
		const struct load_command *lc = (const struct load_command *)cmd;
		if (lc->cmd == LC_SEGMENT_64) {
			const struct segment_command_64 *sc = (const struct segment_command_64 *)lc;
			/* __PAGEZERO also has fileoff==0 but filesize==0 -- the
			 * mach_header only actually lives in the segment that has
			 * real file content backing offset 0. */
			if (sc->fileoff == 0 && sc->filesize > 0) {
				found = sc->vmaddr;
				have = 1;
			}
		}
		cmd += lc->cmdsize;
	}
	if (!have) {
		dyld_panic("main executable: no segment at file offset 0");
	}
	return found;
}

static uint64_t
find_entry_point(const struct mach_header_64 *mh, uint64_t slide)
{
	const uint8_t *cmd = (const uint8_t *)mh + sizeof(*mh);
	for (uint32_t i = 0; i < mh->ncmds; i++) {
		const struct load_command *lc = (const struct load_command *)cmd;
		if (lc->cmd == LC_MAIN) {
			const struct entry_point_command *ep = (const struct entry_point_command *)lc;
			return (uint64_t)(uintptr_t)mh + ep->entryoff;
		}
		cmd += lc->cmdsize;
	}
	(void)slide;
	dyld_panic("main executable: no LC_MAIN (LC_UNIXTHREAD binaries aren't supported)");
}

uint64_t
dyld_bootstrap_main(const struct mach_header_64 *main_mh, int argc, char **argv, char **envp)
{
	(void)argc;
	(void)envp;

	struct image *main_im = &g_images[g_nimages++];
	memset(main_im, 0, sizeof(*main_im));
	strncpy(main_im->path, argv && argv[0] ? argv[0] : "(main)", sizeof(main_im->path) - 1);
	g_main_image = main_im;

	uint64_t preferred = find_preferred_base(main_mh);
	uint64_t slide = (uint64_t)(uintptr_t)main_mh - preferred;

	/* Recursively loads every LC_LOAD_DYLIB dependency (and their own
	 * transitive deps) into g_images[] before we return. */
	image_parse_loaded(main_im, main_mh, slide);

	for (int i = 0; i < g_nimages; i++) {
		image_rebase(&g_images[i]);
	}
	for (int i = 0; i < g_nimages; i++) {
		image_bind(&g_images[i]);
	}

	/* libobjc needs to see every loaded image's Mach-O metadata (class
	 * lists etc.) before anything's global constructors run -- real dyld
	 * does this via a generic registration callback
	 * (_dyld_objc_notify_register) any client can hook; we hardcode the
	 * one client that exists. Deliberately simpler than the real API,
	 * documented as such (see TODO.md Phase 13). */
	for (int i = 0; i < g_nimages; i++) {
		const char *path = g_images[i].path;
		size_t len = strlen(path);
		static const char suffix[] = "/libobjc.A.dylib";
		if (len >= sizeof(suffix) - 1 &&
		    strcmp(path + len - (sizeof(suffix) - 1), suffix) == 0) {
			/* the C function is named _objc_init (matching Apple's real
			 * symbol) -- the leading-underscore C-symbol convention adds
			 * one more on top of that, so the Mach-O export is
			 * __objc_init (confirmed via nm, not guessed). */
			uint64_t objc_init_addr = image_resolve_export(&g_images[i], "__objc_init");
			if (objc_init_addr) {
				const struct mach_header_64 *mhs[IMAGE_MAX];
				for (int j = 0; j < g_nimages; j++) {
					mhs[j] = g_images[j].mh;
				}
				void (*objc_init)(const struct mach_header_64 *const *, int) =
				    (void (*)(const struct mach_header_64 *const *, int))(uintptr_t)objc_init_addr;
				objc_init(mhs, g_nimages);
			}
			break;
		}
	}

	/* Dependencies before dependents, main executable last: a dylib's own
	 * sub-dependencies always land at a strictly higher g_images[] index
	 * than the dylib itself (image_load_dependency appends recursively
	 * loaded deps after registering the parent's own slot), and the main
	 * executable is always index 0 -- so a simple reverse walk gives a
	 * correct-enough ordering without tracking a real dependency graph. */
	for (int i = g_nimages - 1; i >= 0; i--) {
		image_run_mod_init_funcs(&g_images[i]);
	}

	for (int i = 0; i < g_nimages && i < IMAGE_MAX; i++) {
		g_image_info_array[i].imageLoadAddress = (const struct mach_header *)g_images[i].mh;
		g_image_info_array[i].imageFilePath = g_images[i].path;
		g_image_info_array[i].imageFileModDate = 0;
	}
	g_all_image_infos.version = 1;
	g_all_image_infos.infoArrayCount = g_nimages;
	g_all_image_infos.infoArray = g_image_info_array;
	g_all_image_infos.dyldImageLoadAddress = (const struct mach_header *)&_mh_dylinker_header;

	return find_entry_point(main_mh, slide);
}
