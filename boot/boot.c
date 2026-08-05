/*
 * Minimal EFI bootloader for our from-scratch xnu-6153.141.1 kernel.
 * Apple's real boot.efi is not open source; this reimplements just enough
 * of its job: load mach_kernel's Mach-O segments to their physical
 * addresses, build boot_args + a flattened device tree, exit boot
 * services, and hand off to the kernel's real entry point after
 * transitioning back to the 32-bit/no-paging environment it expects (see
 * transition.S). See docs/architecture.md for the full design rationale.
 */
#include "efi.h"
#include "darwin_boot.h"
#include "mach_o.h"

/* ---- direct serial (COM1, 0x3F8) debug output ----
 * independent of whatever the firmware's ConOut is actually wired to --
 * plain port I/O works identically in any x86 mode, so this is a reliable
 * channel for our own bootloader's diagnostics on `-serial mon:stdio`,
 * matching how xnu's own kprintf reaches the same port once we jump in. */
static inline uint8_t inb(uint16_t port)
{
	uint8_t v;
	__asm__ __volatile__("inb %1, %0" : "=a"(v) : "Nd"(port));
	return v;
}

static inline void outb(uint16_t port, uint8_t v)
{
	__asm__ __volatile__("outb %0, %1" : : "a"(v), "Nd"(port));
}

#define COM1 0x3F8

static void serial_init(void)
{
	outb(COM1 + 1, 0x00); /* disable interrupts */
	outb(COM1 + 3, 0x80); /* enable DLAB */
	outb(COM1 + 0, 0x01); /* divisor lo: 115200 baud */
	outb(COM1 + 1, 0x00); /* divisor hi */
	outb(COM1 + 3, 0x03); /* 8N1, DLAB off */
	outb(COM1 + 2, 0xC7); /* enable + clear FIFO */
	outb(COM1 + 4, 0x0B); /* RTS/DSR set */
}

static void serial_putc(char c)
{
	if (c == '\n') {
		serial_putc('\r');
	}
	while (!(inb(COM1 + 5) & 0x20)) {
		/* wait for transmit holding register empty */
	}
	outb(COM1, (uint8_t)c);
}

static void serial_puts(const char *s)
{
	while (*s) {
		serial_putc(*s++);
	}
}

static void serial_puthex64(uint64_t v)
{
	static const char digits[] = "0123456789abcdef";
	serial_puts("0x");
	for (int shift = 60; shift >= 0; shift -= 4) {
		serial_putc(digits[(v >> shift) & 0xF]);
	}
}

/* ---- tiny freestanding helpers (no libc available) ---- */
static void bzero_(void *p, UINTN n)
{
	uint8_t *b = (uint8_t *)p;
	for (UINTN i = 0; i < n; i++) {
		b[i] = 0;
	}
}

static void bcopy_(const void *src, void *dst, UINTN n)
{
	const uint8_t *s = (const uint8_t *)src;
	uint8_t *d = (uint8_t *)dst;
	for (UINTN i = 0; i < n; i++) {
		d[i] = s[i];
	}
}

static UINTN strlen_(const char *s)
{
	UINTN n = 0;
	while (s[n]) {
		n++;
	}
	return n;
}

/* Whitespace-delimited exact-token search (mirrors how xnu's own
 * PE_parse_boot_argn treats "-v": a whole space-separated word, not a
 * substring match -- e.g. a boot-args string containing "vti=0" must NOT
 * count as having "-v"). */
static int cmdline_has_token(const char *cmdline, const char *token)
{
	UINTN tok_len = strlen_(token);
	const char *p = cmdline;
	while (*p) {
		while (*p == ' ') {
			p++;
		}
		if (!*p) {
			break;
		}
		const char *start = p;
		while (*p && *p != ' ') {
			p++;
		}
		UINTN word_len = (UINTN)(p - start);
		if (word_len == tok_len) {
			UINTN i = 0;
			while (i < tok_len && start[i] == token[i]) {
				i++;
			}
			if (i == tok_len) {
				return 1;
			}
		}
	}
	return 0;
}

/* Not cryptographically rigorous -- just needs to be non-trivial entropy
 * for xnu's early PRNG seed (see the device-tree "random-seed" comment
 * below); xnu's own bootseed_init_native mixes in RDSEED/RDRAND itself as
 * an additional source on top of whatever we hand it here. */
static inline uint64_t rdtsc_(void)
{
	uint32_t lo, hi;
	__asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
	return ((uint64_t)hi << 32) | lo;
}

static void fill_random_bytes(uint8_t *buf, UINTN len)
{
	uint64_t state = rdtsc_() ^ 0x9E3779B97F4A7C15ULL;
	for (UINTN i = 0; i < len; i += 8) {
		state ^= rdtsc_();
		state *= 0xBF58476D1CE4E5B9ULL;
		state ^= state >> 27;
		UINTN n = (len - i < 8) ? (len - i) : 8;
		for (UINTN j = 0; j < n; j++) {
			buf[i + j] = (uint8_t)(state >> (j * 8));
		}
	}
}

#define RANDOM_SEED_SIZE 64

/* Flattened-device-tree cursor serializer -- see the "minimal device tree"
 * comment at the call site for why every non-root node needs a "name"
 * property. Property values are padded to a 4-byte boundary, matching
 * next_prop()'s advancement formula in pexpert/gen/device_tree.c:
 *   (prop_addr + prop->length + sizeof(DeviceTreeNodeProperty) + 3) & ~3
 */
static uint8_t *dt_put_node(uint8_t *cursor, uint32_t nProperties, uint32_t nChildren)
{
	DeviceTreeNode *n = (DeviceTreeNode *)cursor;
	n->nProperties = nProperties;
	n->nChildren = nChildren;
	return cursor + sizeof(DeviceTreeNode);
}

static uint8_t *dt_put_prop(uint8_t *cursor, const char *name, const void *value, uint32_t value_len)
{
	DeviceTreeNodeProperty *p = (DeviceTreeNodeProperty *)cursor;
	bzero_(p->name, kPropNameLength);
	bcopy_(name, p->name, strlen_(name));
	p->length = value_len;
	uint8_t *valptr = cursor + sizeof(DeviceTreeNodeProperty);
	bcopy_(value, valptr, value_len);
	uint32_t total = (uint32_t)sizeof(DeviceTreeNodeProperty) + value_len;
	uint32_t padded = (total + 3) & ~3u;
	return cursor + padded;
}

/* noreturn: performs the 64->32 transition and jumps to xnu's entry point;
 * see transition.S. MS x64 ABI: entry/boot_args physical addresses. */
extern void jump_to_kernel(uint32_t entry_phys, uint32_t boot_args_phys) EFIAPI;

#define KERNEL_PATH_U16 u"\\mach_kernel"
#define SPLASH_PATH_U16 u"\\splash.raw"
#define PAGE_SIZE 4096ULL

static EFI_BOOT_SERVICES *gBS;

/* Round up to a page boundary and return page count. */
static UINTN pages_for(uint64_t size)
{
	return (size + PAGE_SIZE - 1) / PAGE_SIZE;
}

/* ---- GOP framebuffer lookup ----
 * OVMF has already picked a mode and owns a linear framebuffer by the time
 * our loader runs (that's how its own ConOut text and the boot logo get
 * drawn) -- we just need to read that mode back via LocateProtocol, no
 * SetMode call needed. If GOP isn't present or is Blt-only (no linear
 * framebuffer), we simply don't populate ba->Video and xnu's own
 * initialize_screen() falls back to serial (see osfmk/console/
 * video_console.c: "No video - forcing serial mode") -- the existing serial
 * console path is left completely untouched as that fallback. */
/* pexpert/pexpert/i386/boot.h: GRAPHICS_MODE(1) is the "pretty" boot-picture
 * console (Apple logo + spinner, text hidden -- what osfmk/console/
 * video_console.c calls gc_graphics_boot, and what real boot.efi only uses
 * for a *non*-verbose boot); FB_TEXT_MODE(2) is the plain scrolling
 * character-grid console real boot.efi switches to for verbose (-v) boot.
 * Which one we pick is now driven by whether the interactive prompt's final
 * command line has "-v" in it (see the KBOOT_GRAPHICS_MODE/show_splash logic
 * in efi_main): verbose wants kernel/BusyBox text visible, not hidden behind
 * the splash. Confirmed by reading initialize_screen()'s kPEAcquireScreen
 * case: with v_display == GRAPHICS_MODE, gc_graphics_boot ends up TRUE, so
 * gc_enable(!graphics_now) resolves to gc_enable(FALSE) -- which explicitly
 * sets disableConsoleOutput = TRUE (console_is_serial() is FALSE for us) --
 * so printf/tty output never reaches the screen at all. FB_TEXT_MODE avoids
 * that branch entirely. */
#define KBOOT_FB_TEXT_MODE 2
/* The other side of that same branch: real boot.efi's non-verbose splash
 * mode, used below (see load_and_blit_splash) when the interactive prompt
 * wasn't given "-v" -- gc_graphics_boot ends up TRUE and text output stays
 * suppressed so it doesn't scribble over the picture we already drew. */
#define KBOOT_GRAPHICS_MODE 1

typedef struct {
	uint64_t base;
	uint32_t width;
	uint32_t height;
	uint32_t rowBytes;
	int valid;
	int bgr; /* 1 => PixelBlueGreenRedReserved8BitPerColor, 0 => RGB order */
} gop_fb_t;

static gop_fb_t locate_gop_framebuffer(void)
{
	gop_fb_t fb = {0};
	EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
	EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = 0;

	EFI_STATUS status = gBS->LocateProtocol(&gop_guid, 0, (void **)&gop);
	if (EFI_ERROR(status) || !gop || !gop->Mode || !gop->Mode->Info) {
		serial_puts("[boot] GOP not found, no framebuffer console\n");
		return fb;
	}

	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info = gop->Mode->Info;
	if (info->PixelFormat != PixelRedGreenBlueReserved8BitPerColor &&
	    info->PixelFormat != PixelBlueGreenRedReserved8BitPerColor) {
		/* PixelBitMask/PixelBltOnly: not the flat 32bpp framebuffer xnu's
		 * boot console expects -- skip rather than guess at a layout. */
		serial_puts("[boot] GOP mode has no usable 32bpp linear framebuffer\n");
		return fb;
	}

	fb.base = (uint64_t)gop->Mode->FrameBufferBase;
	fb.width = info->HorizontalResolution;
	fb.height = info->VerticalResolution;
	fb.rowBytes = info->PixelsPerScanLine * 4;
	fb.bgr = (info->PixelFormat == PixelBlueGreenRedReserved8BitPerColor);
	fb.valid = 1;

	serial_puts("[boot] GOP framebuffer base=");
	serial_puthex64(fb.base);
	serial_puts(" width=");
	serial_puthex64(fb.width);
	serial_puts(" height=");
	serial_puthex64(fb.height);
	serial_puts(" rowBytes=");
	serial_puthex64(fb.rowBytes);
	serial_puts("\n");
	return fb;
}

static EFI_FILE_PROTOCOL *open_esp_root(EFI_HANDLE ImageHandle)
{
	EFI_GUID loaded_image_guid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
	EFI_GUID sfsp_guid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
	EFI_LOADED_IMAGE_PROTOCOL *loaded_image = 0;
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *sfsp = 0;
	EFI_FILE_PROTOCOL *root = 0;
	EFI_STATUS status;

	status = gBS->HandleProtocol(ImageHandle, &loaded_image_guid, (void **)&loaded_image);
	if (EFI_ERROR(status)) {
		serial_puts("HandleProtocol(LoadedImage) failed\n");
		return 0;
	}

	status = gBS->HandleProtocol(loaded_image->DeviceHandle, &sfsp_guid, (void **)&sfsp);
	if (EFI_ERROR(status)) {
		serial_puts("HandleProtocol(SimpleFileSystem) failed\n");
		return 0;
	}

	status = sfsp->OpenVolume(sfsp, &root);
	if (EFI_ERROR(status)) {
		serial_puts("OpenVolume failed\n");
		return 0;
	}
	return root;
}

/* Reads the whole file into an EfiLoaderData pool buffer. Returns 0 on
 * failure. *out_size receives the file size in bytes. */
static EFI_FILE_PROTOCOL *open_kernel_file(EFI_FILE_PROTOCOL *root, CHAR16 *path)
{
	EFI_FILE_PROTOCOL *file = 0;
	EFI_STATUS status = root->Open(root, &file, path, EFI_FILE_MODE_READ, 0);
	if (EFI_ERROR(status)) {
		serial_puts("Open(kernel) failed\n");
		return 0;
	}
	return file;
}

/* Reads just the Mach-O header + load commands (small) into a pool buffer.
 * The segment *data* is read straight into each segment's target physical
 * address later, deliberately never held in a second, arbitrarily-placed
 * buffer -- an earlier version read the whole ~14MB file into one
 * AllocatePool buffer up front and then hit AllocatePages(AllocateAddress)
 * failures (EFI_NOT_FOUND) because that pool buffer landed on top of the
 * low physical addresses the segments themselves need (e.g. __TEXT at
 * 0x200000) -- xnu's segments are linked at fixed low addresses (see
 * KERNEL_HIB_SECTION_BASE in the xnu build), so anything else we allocate
 * before reserving them can collide. */
static void *read_mach_header(EFI_FILE_PROTOCOL *file, UINTN *out_size)
{
	mach_header_64 hdr;
	UINTN n = sizeof(hdr);
	EFI_STATUS status = file->Read(file, &n, &hdr);
	if (EFI_ERROR(status) || n != sizeof(hdr) || hdr.magic != MH_MAGIC_64) {
		serial_puts("Read/parse Mach-O header failed\n");
		return 0;
	}

	UINTN total = sizeof(hdr) + hdr.sizeofcmds;
	void *buf = 0;
	status = gBS->AllocatePool(EfiLoaderData, total, &buf);
	if (EFI_ERROR(status) || !buf) {
		serial_puts("AllocatePool(header) failed\n");
		return 0;
	}

	status = file->SetPosition(file, 0);
	if (EFI_ERROR(status)) {
		serial_puts("SetPosition(0) failed\n");
		gBS->FreePool(buf);
		return 0;
	}
	n = total;
	status = file->Read(file, &n, buf);
	if (EFI_ERROR(status) || n != total) {
		serial_puts("Read(header+cmds) failed\n");
		gBS->FreePool(buf);
		return 0;
	}

	*out_size = total;
	return buf;
}

/* Bump-allocates `pages` pages starting just above the loaded kernel image.
 * boot_args, the device tree, and the memory map buffer are all reached via
 * uint32_t physical-address fields, so everything we hand to xnu must live
 * below the 4GB line -- but "below 4GB" alone isn't enough. AllocateMaxAddress
 * (tried first) satisfies its ceiling by picking the HIGHEST available block
 * under it, so with a 0xFFFFFFFF ceiling it lands these structures near the
 * top of RAM (confirmed via lldb/gdbstub: our device tree ended up at
 * physical ~0x7e797000 with 2048MB of guest RAM). xnu's own early boot-time
 * page tables (built from scratch in osfmk/x86_64/start.s, long before the
 * real pmap/vm subsystem exists) only identity-map a modest low window --
 * high addresses like that are simply not backed by any translation yet,
 * so a later access (from tsc_init's EFI_get_frequency, well after the very
 * first device-tree lookup happened to still be in a live mapping) faults,
 * and since the fault happens before vm_fault_internal's prerequisites
 * (kernel_map, etc.) are ready to service it, the fault-in-a-fault escalates
 * straight to a double fault and then a triple fault. Real EFI bootloaders
 * avoid this by placing these structures in low memory, right after the
 * kernel image -- so we do the same via a simple bump allocator anchored at
 * g_low_alloc_next (initialized by the caller once the kernel's end address
 * is known), using AllocateAddress (a fixed target) instead of
 * AllocateMaxAddress. */
static EFI_PHYSICAL_ADDRESS g_low_alloc_next;

static EFI_STATUS alloc_low(UINTN pages, EFI_PHYSICAL_ADDRESS *out)
{
	EFI_PHYSICAL_ADDRESS addr = g_low_alloc_next;
	EFI_STATUS status = gBS->AllocatePages(AllocateAddress, EfiLoaderData, pages, &addr);
	if (!EFI_ERROR(status)) {
		*out = addr;
		g_low_alloc_next = addr + pages * PAGE_SIZE;
	}
	return status;
}

#define BOOT_PROMPT_BUF_SIZE 256

/* Classic Darwin/boot.efi-style boot-args prompt: shown once, right before
 * ExitBootServices, over the Simple Text Input/Output protocols the
 * firmware already hands us (no new protocol dependency). Backspace edits
 * the line; Enter on an empty line leaves *out untouched (empty string) so
 * the caller's default command line is used as-is. */
static void read_boot_args_prompt(EFI_SYSTEM_TABLE *SystemTable, char *out, UINTN out_size)
{
	EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *co = SystemTable->ConOut;
	EFI_SIMPLE_TEXT_INPUT_PROTOCOL *ci = SystemTable->ConIn;

	/* Best-effort: park the prompt near the bottom of the screen, matching
	 * real boot.efi. Mode/QueryMode/SetCursorPosition are all optional
	 * here (still falls back to wherever the cursor already is) since
	 * some firmware/console combos don't implement them usefully. */
	if (co->Mode && co->QueryMode && co->SetCursorPosition) {
		UINTN cols = 0, rows = 0;
		EFI_STATUS qstatus = co->QueryMode(co, (UINTN)co->Mode->Mode, &cols, &rows);
		if (!EFI_ERROR(qstatus) && rows > 2) {
			co->SetCursorPosition(co, 0, rows - 2);
		}
	}

	co->OutputString(co, u"Boot arguments (Enter to continue):\r\n> ");

	UINTN len = 0;
	for (;;) {
		EFI_INPUT_KEY key;
		EFI_STATUS status;
		do {
			status = ci->ReadKeyStroke(ci, &key);
			if (status == EFI_NOT_READY) {
				gBS->Stall(10000); /* 10ms -- avoid busy-spinning the firmware */
			}
		} while (status == EFI_NOT_READY);

		if (EFI_ERROR(status)) {
			break;
		}

		if (key.UnicodeChar == u'\r' || key.UnicodeChar == u'\n') {
			co->OutputString(co, u"\r\n");
			break;
		}

		if (key.UnicodeChar == 8 || key.UnicodeChar == 0x7F) { /* backspace/DEL */
			if (len > 0) {
				len--;
				co->OutputString(co, u"\b \b");
			}
			continue;
		}

		if (key.UnicodeChar >= 0x20 && key.UnicodeChar < 0x7F && len + 1 < out_size) {
			CHAR16 ch[2] = {key.UnicodeChar, 0};
			out[len++] = (char)key.UnicodeChar;
			co->OutputString(co, ch);
		}
		/* everything else (arrows, function keys, ...) is ignored */
	}
	out[len] = 0;

	serial_puts("[boot] user boot-args: \"");
	serial_puts(out);
	serial_puts("\"\n");
}

/* boot/splash.raw's format: a small fixed header followed by raw RGBA8
 * pixel data (one byte each of R,G,B,A per pixel, row-major, no padding).
 * Pre-resized on the host with a quality resampler by boot/gen_splash.py
 * from boot/assets/splash_source.png -- deliberately NOT a PNG/JPEG: this
 * bootloader has no decoder for either (no zlib inflate, no DCT), and
 * writing one just to show a boot picture is out of scope. Shipping
 * already-decoded, already-correctly-sized pixels means the loader only
 * ever needs a straight memory copy. */
typedef struct {
	char magic[4]; /* "ASPL" */
	uint32_t width;
	uint32_t height;
	uint32_t reserved;
} splash_header_t;

/* Centered, edge-clipped blit -- no scaling. Scaling a raster at boot time
 * with anything simple (nearest-neighbor) is exactly the blocky look we're
 * trying to avoid; boot/gen_splash.py already produced pixels sized for a
 * typical GOP mode, so this only needs to handle the framebuffer being
 * larger (letterbox with the existing black backdrop) or smaller (crop)
 * than that -- both trivial without resampling. */
static void blit_splash(const gop_fb_t *fb, const uint8_t *pixels, uint32_t sw, uint32_t sh)
{
	uint32_t dst_x0 = (fb->width > sw) ? (fb->width - sw) / 2 : 0;
	uint32_t dst_y0 = (fb->height > sh) ? (fb->height - sh) / 2 : 0;
	uint32_t src_x0 = (sw > fb->width) ? (sw - fb->width) / 2 : 0;
	uint32_t src_y0 = (sh > fb->height) ? (sh - fb->height) / 2 : 0;
	uint32_t copy_w = (sw < fb->width) ? sw : fb->width;
	uint32_t copy_h = (sh < fb->height) ? sh : fb->height;

	uint8_t *fb_base = (uint8_t *)(UINTN)fb->base;
	for (uint32_t y = 0; y < copy_h; y++) {
		const uint8_t *srow = pixels + ((uint64_t)(src_y0 + y) * sw + src_x0) * 4;
		uint8_t *drow = fb_base + (uint64_t)(dst_y0 + y) * fb->rowBytes + (uint64_t)dst_x0 * 4;
		for (uint32_t x = 0; x < copy_w; x++) {
			uint8_t r = srow[0], g = srow[1], b = srow[2];
			if (fb->bgr) {
				drow[0] = b;
				drow[1] = g;
				drow[2] = r;
			} else {
				drow[0] = r;
				drow[1] = g;
				drow[2] = b;
			}
			drow[3] = 0;
			srow += 4;
			drow += 4;
		}
	}
}

/* Twelve-dot spinner, horizontally centered under the splash image -- an
 * indeterminate "loading" indicator: this bootloader runs once, before
 * ExitBootServices, with no visibility into how far the kernel/userland
 * boot that follows has actually gotten, so there was never a real
 * progress fraction to draw (the bar this replaced was static for exactly
 * that reason). A spinner doesn't need one either, it just needs to keep
 * moving, so it plays for a fixed short stretch here instead of sitting
 * on screen as a dead image. Skipped rather than drawn off-screen/clipped
 * if the framebuffer is too small to fit the ring below the splash. */
#define SPINNER_DOT_COUNT      (12)
#define SPINNER_DOT_RADIUS     (3)
#define SPINNER_RING_RADIUS    (18)
#define SPINNER_FRAME_COUNT    (24)   /* two full laps */
#define SPINNER_FRAME_STALL_US (70000)

/* cos/sin of i * 30 degrees, fixed point (unit circle * 256) */
static const int32_t spinner_dx256[SPINNER_DOT_COUNT] = {
	 256,  222,  128,    0, -128, -222, -256, -222, -128,    0,  128,  222
};
static const int32_t spinner_dy256[SPINNER_DOT_COUNT] = {
	   0,  128,  222,  256,  222,  128,    0, -128, -222, -256, -222, -128
};

static void put_gray_pixel(const gop_fb_t *fb, uint32_t x, uint32_t y, uint8_t v)
{
	uint8_t *drow = (uint8_t *)(UINTN)fb->base + (uint64_t)y * fb->rowBytes + (uint64_t)x * 4;
	drow[0] = v;
	drow[1] = v;
	drow[2] = v;
	drow[3] = 0;
}

/* headIndex is which dot is brightest; the rest fade out going backwards
 * around the ring, same comet-tail look as any indeterminate spinner. */
static void blit_spinner_frame(const gop_fb_t *fb, uint32_t cx, uint32_t cy, int headIndex)
{
	uint32_t half = SPINNER_RING_RADIUS + SPINNER_DOT_RADIUS + 2;
	uint32_t box_x0 = cx - half;
	uint32_t box_y0 = cy - half;

	/* repaint the whole box black first -- the backdrop here is
	 * guaranteed flat black (same assumption blit_splash's letterboxing
	 * already relies on), so this is a cheap full erase rather than
	 * needing to save/restore whatever was there before */
	for (uint32_t y = 0; y < half * 2; y++) {
		bzero_((uint8_t *)(UINTN)fb->base + (uint64_t)(box_y0 + y) * fb->rowBytes + (uint64_t)box_x0 * 4,
		       (UINTN)half * 2 * 4);
	}

	for (int i = 0; i < SPINNER_DOT_COUNT; i++) {
		int dotIndex = (i - headIndex + SPINNER_DOT_COUNT) % SPINNER_DOT_COUNT;
		uint8_t brightness = (uint8_t)(0xFF - ((0xFF * dotIndex) / (SPINNER_DOT_COUNT - 1)));
		int32_t dcx = (int32_t)cx + ((SPINNER_RING_RADIUS * spinner_dx256[i]) / 256);
		int32_t dcy = (int32_t)cy + ((SPINNER_RING_RADIUS * spinner_dy256[i]) / 256);

		for (int y = -SPINNER_DOT_RADIUS; y <= SPINNER_DOT_RADIUS; y++) {
			for (int x = -SPINNER_DOT_RADIUS; x <= SPINNER_DOT_RADIUS; x++) {
				if (x * x + y * y <= SPINNER_DOT_RADIUS * SPINNER_DOT_RADIUS) {
					put_gray_pixel(fb, (uint32_t)(dcx + x), (uint32_t)(dcy + y), brightness);
				}
			}
		}
	}
}

static void blit_loading_spinner(const gop_fb_t *fb, uint32_t image_bottom_y)
{
	uint32_t margin_top = 18;
	uint32_t half = SPINNER_RING_RADIUS + SPINNER_DOT_RADIUS + 2;
	uint32_t cx = fb->width / 2;
	uint32_t cy = image_bottom_y + margin_top + half;

	if (half > fb->width / 2 || cy + half > fb->height) {
		return; /* doesn't fit -- leave the splash alone rather than clip it oddly */
	}

	for (int frame = 0; frame < SPINNER_FRAME_COUNT; frame++) {
		blit_spinner_frame(fb, cx, cy, frame % SPINNER_DOT_COUNT);
		gBS->Stall(SPINNER_FRAME_STALL_US);
	}
}

/* Loads \splash.raw from the ESP and blits it onto the already-located GOP
 * framebuffer, then plays a loading spinner under it. Returns 1 on success
 * (caller should then select KBOOT_GRAPHICS_MODE), 0 on any failure
 * (missing file, bad header, short read) so the caller can fall back to
 * the always-visible text console instead of handing the user a blank
 * black screen with no explanation. */
static int load_and_blit_splash(EFI_FILE_PROTOCOL *root, const gop_fb_t *fb)
{
	EFI_FILE_PROTOCOL *file = 0;
	EFI_STATUS status = root->Open(root, &file, (CHAR16 *)SPLASH_PATH_U16, EFI_FILE_MODE_READ, 0);
	if (EFI_ERROR(status)) {
		serial_puts("[boot] splash.raw not found, skipping splash\n");
		return 0;
	}

	uint8_t info_buf[512];
	UINTN info_size = sizeof(info_buf);
	EFI_GUID file_info_guid = EFI_FILE_INFO_GUID;
	status = file->GetInfo(file, &file_info_guid, &info_size, info_buf);
	if (EFI_ERROR(status)) {
		serial_puts("[boot] GetInfo(splash.raw) failed\n");
		file->Close(file);
		return 0;
	}
	UINTN file_size = (UINTN)((EFI_FILE_INFO *)info_buf)->FileSize;
	if (file_size <= sizeof(splash_header_t)) {
		serial_puts("[boot] splash.raw too small\n");
		file->Close(file);
		return 0;
	}

	void *buf = 0;
	status = gBS->AllocatePool(EfiLoaderData, file_size, &buf);
	if (EFI_ERROR(status) || !buf) {
		serial_puts("[boot] AllocatePool(splash) failed\n");
		file->Close(file);
		return 0;
	}

	UINTN n = file_size;
	status = file->Read(file, &n, buf);
	file->Close(file);
	if (EFI_ERROR(status) || n != file_size) {
		serial_puts("[boot] Read(splash.raw) failed\n");
		gBS->FreePool(buf);
		return 0;
	}

	splash_header_t *hdr = (splash_header_t *)buf;
	if (hdr->magic[0] != 'A' || hdr->magic[1] != 'S' || hdr->magic[2] != 'P' || hdr->magic[3] != 'L') {
		serial_puts("[boot] splash.raw bad magic\n");
		gBS->FreePool(buf);
		return 0;
	}

	UINTN pixel_bytes = (UINTN)hdr->width * (UINTN)hdr->height * 4;
	if (file_size - sizeof(splash_header_t) < pixel_bytes) {
		serial_puts("[boot] splash.raw truncated\n");
		gBS->FreePool(buf);
		return 0;
	}

	serial_puts("[boot] blitting splash ");
	serial_puthex64(hdr->width);
	serial_puts("x");
	serial_puthex64(hdr->height);
	serial_puts("\n");
	blit_splash(fb, (const uint8_t *)(hdr + 1), hdr->width, hdr->height);

	/* Same centering math as blit_splash's dst_y0 -- recomputed here
	 * rather than threaded back out through a return value, to find
	 * where the image's bottom edge actually landed (letterboxed,
	 * cropped, or exact) so the spinner goes directly under it. */
	uint32_t dst_y0 = (fb->height > hdr->height) ? (fb->height - hdr->height) / 2 : 0;
	uint32_t copy_h = (hdr->height < fb->height) ? hdr->height : fb->height;
	blit_loading_spinner(fb, dst_y0 + copy_h);

	gBS->FreePool(buf);
	return 1;
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
	gBS = SystemTable->BootServices;
	serial_init();
	serial_puts("\n[boot] AsterOS EFI loader starting\n");

	SystemTable->ConOut->OutputString(SystemTable->ConOut, u"AsterOS EFI loader\r\n");

	gop_fb_t gop_fb = locate_gop_framebuffer();

	EFI_FILE_PROTOCOL *root = open_esp_root(ImageHandle);
	if (!root) {
		serial_puts("[boot] failed to open ESP root\n");
		return EFI_LOAD_ERROR;
	}

	EFI_FILE_PROTOCOL *kfile_h = open_kernel_file(root, (CHAR16 *)KERNEL_PATH_U16);
	if (!kfile_h) {
		serial_puts("[boot] failed to open mach_kernel\n");
		return EFI_LOAD_ERROR;
	}

	UINTN hdr_size = 0;
	void *hdr_buf = read_mach_header(kfile_h, &hdr_size);
	if (!hdr_buf) {
		serial_puts("[boot] failed to read Mach-O header\n");
		return EFI_LOAD_ERROR;
	}
	serial_puts("[boot] read Mach-O header+cmds, size=");
	serial_puthex64(hdr_size);
	serial_puts("\n");

	mach_header_64 *mh = (mach_header_64 *)hdr_buf;

	uint64_t entry_virt = 0;
	int have_entry = 0;
	uint64_t kaddr_low32 = 0xFFFFFFFFULL;
	uint64_t kend_low32 = 0;
	int have_seg = 0;

	uint8_t *cmd_ptr = (uint8_t *)hdr_buf + sizeof(mach_header_64);
	for (uint32_t i = 0; i < mh->ncmds; i++) {
		load_command *lc = (load_command *)cmd_ptr;

		if (lc->cmd == LC_SEGMENT_64) {
			segment_command_64 *seg = (segment_command_64 *)lc;
			uint32_t seg_phys = (uint32_t)(seg->vmaddr & 0xFFFFFFFFu);
			uint32_t seg_pages = (uint32_t)pages_for(seg->vmsize);

			serial_puts("[boot] segment ");
			serial_puts(seg->segname);
			serial_puts(" vmaddr=");
			serial_puthex64(seg->vmaddr);
			serial_puts(" vmsize=");
			serial_puthex64(seg->vmsize);
			serial_puts(" phys=");
			serial_puthex64(seg_phys);
			serial_puts("\n");

			if (seg->vmsize == 0) {
				cmd_ptr += lc->cmdsize;
				continue;
			}

			/* Reserve this segment's physical pages *before* reading
			 * anything else from the file, so no other allocation can
			 * land on top of it first (see read_mach_header's comment). */
			EFI_PHYSICAL_ADDRESS addr = seg_phys;
			EFI_STATUS status = gBS->AllocatePages(AllocateAddress, EfiLoaderData, seg_pages, &addr);
			if (EFI_ERROR(status)) {
				serial_puts("[boot] AllocatePages(segment) failed: ");
				serial_puthex64(status);
				serial_puts("\n");
				return EFI_LOAD_ERROR;
			}

			/* Read the segment's file-backed bytes straight into its
			 * target physical address, then zero the bss remainder. */
			if (seg->filesize > 0) {
				status = kfile_h->SetPosition(kfile_h, seg->fileoff);
				if (EFI_ERROR(status)) {
					serial_puts("[boot] SetPosition(segment) failed\n");
					return EFI_LOAD_ERROR;
				}
				UINTN n = (UINTN)seg->filesize;
				status = kfile_h->Read(kfile_h, &n, (void *)(UINTN)seg_phys);
				if (EFI_ERROR(status) || n != (UINTN)seg->filesize) {
					serial_puts("[boot] Read(segment) failed\n");
					return EFI_LOAD_ERROR;
				}
			}
			if (seg->vmsize > seg->filesize) {
				bzero_((void *)(UINTN)(seg_phys + seg->filesize), (UINTN)(seg->vmsize - seg->filesize));
			}

			if (seg_phys < kaddr_low32) {
				kaddr_low32 = seg_phys;
			}
			if ((uint64_t)seg_phys + seg->vmsize > kend_low32) {
				kend_low32 = (uint64_t)seg_phys + seg->vmsize;
			}
			have_seg = 1;
		} else if (lc->cmd == LC_UNIXTHREAD) {
			thread_command *tc = (thread_command *)lc;
			if (tc->flavor == x86_THREAD_STATE64) {
				x86_thread_state64_t *state = (x86_thread_state64_t *)((uint8_t *)tc + sizeof(thread_command));
				entry_virt = state->rip;
				have_entry = 1;
				serial_puts("[boot] entry rip=");
				serial_puthex64(entry_virt);
				serial_puts("\n");
			}
		}

		cmd_ptr += lc->cmdsize;
	}

	kfile_h->Close(kfile_h);

	if (!have_entry || !have_seg) {
		serial_puts("[boot] missing entry point or segments\n");
		return EFI_LOAD_ERROR;
	}

	uint32_t entry_phys = (uint32_t)(entry_virt & 0xFFFFFFFFu);
	uint32_t kaddr = (uint32_t)kaddr_low32;

	/* Where to put boot_args/the device tree/the memory-map buffer took
	 * three attempts, each ground-truthed by reading osfmk/i386/i386_init.c
	 * and osfmk/x86_64/pmap.c against an actual failure caught with
	 * lldb/gdbstub:
	 *
	 * 1. Right at kend_low32, reported as ordinary EfiLoaderData: xnu's
	 *    vstart() sets `physfree = kaddr + ksize` (i386_init.c:606-607)
	 *    and Idle_PTs_init() ALLOCPAGES()-bumps physfree upward from
	 *    there (bzeroing each block) to carve out its own bootstrap page
	 *    tables. Since our data started at exactly kaddr+ksize, xnu's own
	 *    bootstrap allocator hands out (and zeroes) that same memory for
	 *    itself before our device tree is ever read.
	 *
	 * 2. kaddr+16MB: dodged that specific collision (xnu's bootstrap
	 *    growth didn't reach that far), but Idle_PTs_init's fillkpt()
	 *    call (i386_init.c:371-373) only identity-maps physical
	 *    [0, physfree) as it stood at that exact moment, and
	 *    postcode(VSTART_SET_CR3) switches to that page table shortly
	 *    after -- any generous mapping the bootloader/xnu's own low-level
	 *    start.s setup provided before that is gone. +16MB of slack
	 *    happened to exceed physfree's growth, so it was never mapped at
	 *    all: a later device-tree read (tsc_init -> EFI_get_frequency ->
	 *    DTLookupEntry) triple-faulted on a genuinely unmapped access.
	 *
	 * 3. kaddr-4MB (below the kernel, since fillkpt's mapping always
	 *    starts at physical address 0, so anything below kaddr is
	 *    unconditionally inside [0, physfree) no matter how much
	 *    bootstrap growth happens above it): this fixed the mapping
	 *    problem, but introduced a *content* problem instead.
	 *    pmap_lowmem_finalize() (osfmk/x86_64/pmap.c) frees every
	 *    pmap_memory_region entirely below vm_kernel_base_page (=kaddr)
	 *    back to the general allocator -- our region qualified, and
	 *    xnu kept using boot_args (PE_state.bootArgs) long after. Marking
	 *    the descriptor EFI_MEMORY_KERN_RESERVED stopped THAT free call,
	 *    but a direct memory read via lldb proved the page still got
	 *    reused anyway (the first 40 bytes of our device tree were
	 *    overwritten with unrelated pointer-looking data, timed to
	 *    "zalloc: allocating memory for zone names buffer") --
	 *    EFI_MEMORY_KERN_RESERVED only ever meant "don't hand this range
	 *    to devices for DMA," not "no part of xnu's own allocator stack
	 *    may reuse it." Reclassifying the descriptor as EfiACPIMemoryNVS
	 *    (which i386_vm_init.c excludes from pmap_memory_regions
	 *    entirely) hit the identical crash again, so evidently some
	 *    OTHER early allocator -- zalloc's zone bootstrap is the prime
	 *    suspect given the log timing -- doesn't consult the EFI memory
	 *    map's type/attribute at all.
	 *
	 * Real fix: don't try to convince xnu's allocators to leave a region
	 * alone via EFI metadata they don't all honor -- place the data
	 * where xnu ALREADY refuses to touch it unconditionally: inside its
	 * own reported kernel image. kend_low32 (immediately after the real
	 * Mach-O segments) is exactly where "kernel image" ends per
	 * boot_args, so anchoring the bump allocator there with zero gap and
	 * then reporting `ksize` as the REAL kernel size plus everything we
	 * bump-allocated (see the ba->ksize assignment below, after all the
	 * low_alloc() calls) makes our whole region part of [kaddr, kaddr+
	 * ksize) from xnu's point of view. That range is never freed by
	 * pmap_lowmem_finalize (vm_kernel_base_page = kaddr, and the free
	 * loop only touches regions with end < vm_kernel_base_page -- the
	 * kernel's own image starts AT vm_kernel_base_page, so it can never
	 * qualify), and physfree = kaddr+ksize means xnu's OWN bootstrap
	 * allocator growth starts only after our data too, avoiding failure
	 * mode #1 above for free. No EFI attribute or type games needed. */
	g_low_alloc_next = kend_low32;

	gBS->FreePool(hdr_buf);

	/* ---- boot_args (must be below 4GB: uint32_t field in xnu) ---- */
	EFI_PHYSICAL_ADDRESS ba_phys;
	if (EFI_ERROR(alloc_low(pages_for(sizeof(boot_args)), &ba_phys))) {
		serial_puts("[boot] failed to allocate boot_args\n");
		return EFI_LOAD_ERROR;
	}
	boot_args *ba = (boot_args *)(UINTN)ba_phys;
	bzero_(ba, sizeof(*ba));

	/* ---- RAMDisk: a real FAT16 filesystem image (boot/fat16.img),
	 * loaded whole. xnu's IOFindBSDRoot() (iokit/bsddev/IOKitBSDInit.cpp)
	 * looks for a "RAMDisk" property under /chosen/memory-map --
	 * {uint64_t phys_base, uint64_t size} -- and if present, calls
	 * mdevadd() to create /dev/md0 from it. Our own bsd/miscfs/fat16lite
	 * driver (tried before mockfs in vfs_conf.c's mountroot table) then
	 * parses this memory device directly as a FAT16 volume -- the
	 * required persistent-root layout (/bin/busybox, /sbin/init, /dev,
	 * /etc, /tmp, /usr, /var; see docs/architecture.md) lives inside it,
	 * not just a single raw executable as with the earlier mockfs-only
	 * RAMDisk (still available as a fallback if this doesn't look like a
	 * valid FAT16 image -- see fat16lite_mountroot's BPB signature
	 * check). */
	EFI_PHYSICAL_ADDRESS ramdisk_phys = 0;
	UINTN ramdisk_size = 0;
	{
		EFI_FILE_PROTOCOL *rd_file = 0;
		EFI_STATUS status = root->Open(root, &rd_file, (CHAR16 *)u"\\fat16.img", EFI_FILE_MODE_READ, 0);
		if (EFI_ERROR(status)) {
			serial_puts("[boot] failed to open fat16.img\n");
			return EFI_LOAD_ERROR;
		}

		uint8_t info_buf[512];
		UINTN info_size = sizeof(info_buf);
		EFI_GUID file_info_guid = EFI_FILE_INFO_GUID;
		status = rd_file->GetInfo(rd_file, &file_info_guid, &info_size, info_buf);
		if (EFI_ERROR(status)) {
			serial_puts("[boot] GetInfo(fat16.img) failed\n");
			return EFI_LOAD_ERROR;
		}
		EFI_FILE_INFO *info = (EFI_FILE_INFO *)info_buf;
		UINTN file_size = (UINTN)info->FileSize;

		UINTN rd_pages = pages_for(file_size);
		if (EFI_ERROR(alloc_low(rd_pages, &ramdisk_phys))) {
			serial_puts("[boot] failed to allocate RAMDisk\n");
			return EFI_LOAD_ERROR;
		}
		/* size advertised to xnu must be a whole number of pages --
		 * mdevadd() computes its page count as `ramdParms[1] >> 12`,
		 * which would silently truncate a partial trailing page if we
		 * advertised the exact (non-page-aligned) file size. */
		ramdisk_size = rd_pages * PAGE_SIZE;
		bzero_((void *)(UINTN)ramdisk_phys, ramdisk_size);

		UINTN n = file_size;
		status = rd_file->Read(rd_file, &n, (void *)(UINTN)ramdisk_phys);
		if (EFI_ERROR(status) || n != file_size) {
			serial_puts("[boot] Read(fat16.img) failed\n");
			return EFI_LOAD_ERROR;
		}
		rd_file->Close(rd_file);

		serial_puts("[boot] loaded fat16.img RAMDisk phys=");
		serial_puthex64((uint64_t)ramdisk_phys);
		serial_puts(" size=");
		serial_puthex64(ramdisk_size);
		serial_puts("\n");
	}

	/* ---- interactive boot-args prompt: kernel + RAMDisk are loaded, boot
	 * services (and thus the console) are still up, and we haven't touched
	 * the default command line yet -- exactly the classic boot.efi moment
	 * for this. An empty line (bare Enter) leaves user_boot_args empty, so
	 * the default command line below is used unmodified. */
	char user_boot_args[BOOT_PROMPT_BUF_SIZE];
	read_boot_args_prompt(SystemTable, user_boot_args, sizeof(user_boot_args));

	/* ---- minimal device tree: root -> chosen(random-seed) -> memory-map
	 * (RAMDisk).
	 *
	 * Two things ground-truthed the hard way (attaching lldb to QEMU's
	 * gdbstub and breaking on `panic`) that a naive reading of
	 * pexpert/device_tree.h doesn't tell you:
	 *
	 * 1. The "random-seed" property under /chosen is NOT optional: xnu's
	 *    early PRNG init (osfmk/prng/prng_random.c,
	 *    bootseed_init_bootloader) calls PE_get_random_seed(), which
	 *    reads exactly this property, and panics ("Expected %lu seed
	 *    bytes from bootloader, but got %u.") if it's missing or short.
	 *
	 * 2. Path lookups (DTLookupEntry(NULL, "/chosen", ...), used by
	 *    PE_get_random_seed itself) walk children by matching a "name"
	 *    property via strcmp (pexpert/gen/device_tree.c: FindChild) --
	 *    NOT by structural position. A node with zero properties can
	 *    never be found by path, even if it's structurally exactly where
	 *    you'd expect "/chosen" to be: DTGetProperty(child, "name", ...)
	 *    fails immediately and FindChild gives up. So every node from the
	 *    root down to whatever we want reachable via a path string needs
	 *    an explicit "name" property matching that path component.
	 *
	 * 3. nProperties == 0 is NOT "a valid node with no properties" -- it's
	 *    an explicit end-of-list sentinel. skipProperties() (used by
	 *    GetFirstChild/GetNextChild) returns NULL the instant it sees
	 *    nProperties == 0, and find_entry()'s own comment confirms it:
	 *    "if (nodeP->nProperties == 0) return kError; // End of the list
	 *    of nodes". A root node with nProperties=0 therefore can never be
	 *    descended into -- GetFirstChild(root) returns NULL before ever
	 *    looking at nChildren, so /chosen is unreachable no matter how
	 *    correct the rest of the tree is. Every node, including the root,
	 *    needs at least one real property (we give root a "name" too).
	 */
	EFI_PHYSICAL_ADDRESS dt_phys;
	UINTN dt_pages = 1; /* one page is enormously more than this tree needs */
	if (EFI_ERROR(alloc_low(dt_pages, &dt_phys))) {
		serial_puts("[boot] failed to allocate device tree\n");
		return EFI_LOAD_ERROR;
	}
	uint8_t *dt_base = (uint8_t *)(UINTN)dt_phys;
	bzero_(dt_base, dt_pages * PAGE_SIZE);

	uint8_t *cur = dt_base;
	uint8_t *root_node = cur;
	cur = dt_put_node(cur, 1, 1); /* root: 1 prop (name), 1 child (chosen) */
	cur = dt_put_prop(cur, "name", "device-tree", strlen_("device-tree") + 1);

	cur = dt_put_node(cur, 2, 1); /* chosen: 2 props, 1 child (memory-map) */
	cur = dt_put_prop(cur, "name", "chosen", strlen_("chosen") + 1);
	{
		uint8_t seed[RANDOM_SEED_SIZE];
		fill_random_bytes(seed, sizeof(seed));
		cur = dt_put_prop(cur, "random-seed", seed, sizeof(seed));
	}

	cur = dt_put_node(cur, 2, 0); /* memory-map: 2 props, 0 children */
	cur = dt_put_prop(cur, "name", "memory-map", strlen_("memory-map") + 1);
	{
		uint64_t ramd_parms[2];
		ramd_parms[0] = (uint64_t)ramdisk_phys;
		ramd_parms[1] = (uint64_t)ramdisk_size;
		cur = dt_put_prop(cur, "RAMDisk", ramd_parms, sizeof(ramd_parms));
	}

	UINTN dt_size = (UINTN)(cur - root_node);

	/* ---- memory map buffer (generous fixed size; grown below if needed) ---- */
	UINTN mmap_pages = 8;
	EFI_PHYSICAL_ADDRESS mmap_phys;
	if (EFI_ERROR(alloc_low(mmap_pages, &mmap_phys))) {
		serial_puts("[boot] failed to allocate memory map buffer\n");
		return EFI_LOAD_ERROR;
	}

	ba->Revision = kBootArgsRevision;
	ba->Version = kBootArgsVersion2;
	ba->efiMode = kBootArgsEfiMode64;
	{
		/* rd=md0: IOFindBSDRoot (iokit/bsddev/IOKitBSDInit.cpp) only
		 * *creates* /dev/md0 automatically from the device tree's
		 * RAMDisk property -- actually *selecting* it as the root
		 * device still requires this boot-arg (its "rd=mdX"/
		 * "rootdev=mdX" parsing is what sets *root, unconditionally
		 * on every xnu build, not gated by any config option).
		 *
		 * vti=0: xnu detects any hypervisor (CPUID_FEATURE_VMM) and
		 * inflates most kernel timeouts by <<6 (64x) by default --
		 * standard, intentional behavior (osfmk/i386/machine_routines.c),
		 * not a bug -- but it's exactly why IOFindBSDRoot's 30-second
		 * wait for the IOBSD resource turned into many real-world
		 * minutes under QEMU/TCG's slow, software-emulated TSC. Since
		 * this is purely a test/development environment (not
		 * "production" virtualization with unpredictable host
		 * contention), disabling the inflation via this already-
		 * provided boot-arg keeps the test/rebuild loop fast without
		 * touching any kernel logic. */
		/* serial=2 (SERIALMODE_INPUT only, no SERIALMODE_OUTPUT): keeps
		 * the serial line as a redundant/fallback keyboard input path
		 * (osfmk/console/serial_general.c's serial_keyboard_init(), same
		 * as before) alongside the PS/2 poller (osfmk/console/
		 * ps2_kbd.c), but no longer forces switch_to_serial_console() in
		 * osfmk/i386/i386_init.c -- that call is gated on
		 * SERIALMODE_OUTPUT, and dropping it leaves cons_ops_index at its
		 * compiled-in default (VC_CONS_OPS, serial_console.c), so BSD
		 * console/tty output (printf, the BusyBox tty) goes to the GOP
		 * framebuffer console once ba->Video is populated below, and
		 * falls back to serial automatically (initialize_screen's own
		 * "No video - forcing serial mode" path) if it isn't.
		 *
		 * debug=0x14C adds DB_KPRT (0x8) on top of the existing 0x144:
		 * without it, PE_kputc (pe_kprintf.c) resolves to cnputc, so
		 * kprintf() -- the low-level `-v` boot-message channel, distinct
		 * from printf() -- would follow cons_ops_index onto the
		 * framebuffer too and be lost if anything goes wrong before the
		 * console is up. With DB_KPRT set, PE_kputc is pal_serial_putc
		 * instead: kprintf output goes straight to the raw serial UART
		 * unconditionally, independent of cons_ops_index, so serial stays
		 * a live kernel debug log for the whole boot regardless of what
		 * the graphical console is doing. */
		/* No "-v" here on purpose: verbosity is now the prompt's call (see
		 * read_boot_args_prompt above and the KBOOT_GRAPHICS_MODE/splash
		 * selection below) -- a bare Enter should mean a quiet, splash-
		 * screen boot, not a forced-verbose one. */
		/* vm_compressor=2 (VM_PAGER_COMPRESSOR_NO_SWAP, osfmk/vm/vm_pageout.h):
		 * in-RAM compressor only, no disk-swap backend. The default
		 * (VM_PAGER_COMPRESSOR_WITH_SWAP) has the compressor try to create
		 * /private/var/vm/swapfileN on the root filesystem -- nonsensical
		 * here since root is itself a RAMDisk (compressing pages just to
		 * write them onto more RAM pretending to be a disk), and it fails
		 * outright anyway: bsd/miscfs/fat16lite has no vnode_setsize/
		 * preallocate support, which swap file creation requires. Without
		 * this, that failure is merely noisy (retried lazily, never
		 * blocks boot -- see vm_swap_create_file's callers), but there's
		 * no reason to let it try something that can't work. */
		const char *default_cmdline = "keepsyms=1 debug=0x14C rd=md0 vti=0 serial=2 vm_compressor=2";
		UINTN n = strlen_(default_cmdline);
		if (n >= BOOT_LINE_LENGTH) {
			n = BOOT_LINE_LENGTH - 1;
		}
		bcopy_(default_cmdline, ba->CommandLine, n);

		/* Append rather than replace: rd=md0/serial=2/etc above are load-
		 * bearing (see the comments just below), so a user-entered "-v" or
		 * "keepsyms=1" at the prompt extends the default line instead of
		 * losing them. */
		UINTN user_len = strlen_(user_boot_args);
		if (user_len > 0 && n < BOOT_LINE_LENGTH - 1) {
			ba->CommandLine[n++] = ' ';
			UINTN avail = BOOT_LINE_LENGTH - 1 - n;
			if (user_len > avail) {
				user_len = avail;
			}
			bcopy_(user_boot_args, ba->CommandLine + n, user_len);
			n += user_len;
		}
		ba->CommandLine[n] = 0;

		serial_puts("[boot] final CommandLine: \"");
		serial_puts(ba->CommandLine);
		serial_puts("\"\n");
	}
	/* "-v" is a whole boot-arg token, same as xnu's own parser -- checked
	 * against the final, already-appended CommandLine so it doesn't matter
	 * whether it came from the default or the user typed it at the
	 * prompt. */
	int verbose = cmdline_has_token(ba->CommandLine, "-v");
	if (gop_fb.valid) {
		/* pe_init.c's PE_init_platform() prefers ba->Video (the
		 * "new EFI-style" struct) over VideoV1 whenever v_baseAddr != 0,
		 * so VideoV1 is deliberately left zeroed. v_rotate=0/v_scale are
		 * also zero-initialized (bzero_(ba, ...) above), which pe_init.c
		 * reads as kDataRotate0/kPEScaleFactor1x -- correct for a plain
		 * QEMU framebuffer, no rotation or HiDPI scale. */
		ba->Video.v_baseAddr = gop_fb.base;
		ba->Video.v_rowBytes = gop_fb.rowBytes;
		ba->Video.v_width = gop_fb.width;
		ba->Video.v_height = gop_fb.height;
		ba->Video.v_depth = 32;

		/* Quiet boot (no "-v"): paint the splash + a loading spinner and use
		 * KBOOT_GRAPHICS_MODE so xnu's console leaves it alone while
		 * boot runs.
		 *
		 * GRAPHICS_MODE used to be a one-way trip in this kernel:
		 * kernel_bootstrap_thread()'s one-shot initialize_screen(NULL,
		 * kPEAcquireScreen) call (the AsterOS addition that stands in
		 * for a real IOFramebuffer driver attaching) latches
		 * disableConsoleOutput = TRUE for that mode, and nothing ever
		 * called kPETextScreen to undo it -- there's no WindowServer/
		 * loginwindow here to reveal the shell once boot finishes the
		 * way real macOS's boot picture does. That's now fixed on the
		 * userland side instead: launchd (userland/launchd/launchd.c)
		 * writes to the kern.consoletext sysctl (bsd/kern/kern_sysctl.c)
		 * right before starting the shell daemon, which calls
		 * initialize_screen(NULL, kPETextScreen) and un-hides the
		 * console at exactly the right moment. So it's safe to hide
		 * text behind the splash here again. */
		if (!verbose) {
			/* Wipe whatever firmware boot-log text and our own prompt
			 * left on the (text-mode, but GOP-backed) console before
			 * painting pixels directly -- otherwise stray characters from
			 * earlier in boot sit on top of/near the splash. */
			if (SystemTable->ConOut->ClearScreen) {
				SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
			}
			load_and_blit_splash(root, &gop_fb);
		}
		ba->Video.v_display = !verbose ? KBOOT_GRAPHICS_MODE : KBOOT_FB_TEXT_MODE;
	}

	ba->deviceTreeP = (uint32_t)dt_phys;
	ba->deviceTreeLength = (uint32_t)dt_size;
	ba->kaddr = kaddr;
	/* Extend past the real Mach-O footprint to also cover boot_args/the
	 * device tree/the memory-map buffer -- see the g_low_alloc_next
	 * comment above for why. g_low_alloc_next has been bumped past all
	 * three by this point (mmap_phys, the last of them, was allocated
	 * just above). */
	ba->ksize = (uint32_t)(g_low_alloc_next - kaddr_low32);

	/* ---- get the memory map, then immediately ExitBootServices with the
	 * same key. No allocations may happen between GetMemoryMap and
	 * ExitBootServices or the key goes stale; retry a few times since
	 * ExitBootServices legitimately fails once in a while if firmware
	 * background activity changed the map underneath us. */
	EFI_STATUS status = EFI_LOAD_ERROR;
	for (int attempt = 0; attempt < 4; attempt++) {
		UINTN mmap_size = mmap_pages * PAGE_SIZE;
		UINTN map_key = 0, desc_size = 0;
		uint32_t desc_version = 0;

		status = gBS->GetMemoryMap(&mmap_size, (EFI_MEMORY_DESCRIPTOR *)(UINTN)mmap_phys, &map_key, &desc_size, &desc_version);
		if (EFI_ERROR(status)) {
			serial_puts("[boot] GetMemoryMap failed\n");
			return EFI_LOAD_ERROR;
		}

		ba->MemoryMap = (uint32_t)mmap_phys;
		ba->MemoryMapSize = (uint32_t)mmap_size;
		ba->MemoryMapDescriptorSize = (uint32_t)desc_size;
		ba->MemoryMapDescriptorVersion = desc_version;

		{
			/* EFI_MEMORY_RUNTIME (bit 63 of Attribute) -- see UEFI spec
			 * "Memory Attribute Definitions". Descriptors with this bit
			 * set are meant to keep a valid mapping after
			 * ExitBootServices, normally arranged by calling
			 * SetVirtualAddressMap() to give each one a real
			 * VirtualStart. We deliberately never call that (this
			 * kernel has no use for EFI runtime services -- no NVRAM/
			 * variable access, no reboot-via-firmware, consistent with
			 * the rest of the "no full macOS" scope), which leaves
			 * VirtualStart at 0 for every descriptor.
			 *
			 * xnu's efi_init() (osfmk/i386/AT386/model_dep.c) doesn't
			 * know that: for every EFI_MEMORY_RUNTIME descriptor it does
			 * `vm_addr = VirtualStart; if (vm_addr < VM_MIN_KERNEL_
			 * ADDRESS) vm_addr |= VM_MIN_KERNEL_ADDRESS;` then calls
			 * pmap_map_bd(vm_addr, PhysicalStart, ...). With
			 * VirtualStart==0 for everything, EVERY runtime region
			 * mapped to the exact same virtual address (VM_MIN_KERNEL_
			 * ADDRESS, i.e. virtual page 0) -- each one overwriting the
			 * last, and whichever region was processed last (some OVMF
			 * firmware region near 0xFFC00000, empirically) permanently
			 * clobbered the identity map virtual pages 0-2MB depend on.
			 * Confirmed via a bisection of raw-serial checkpoints across
			 * Idle_PTs_init/machine_init/efi_init: KPTphys[0..3] read
			 * back correctly (0x3/0x1003/0x2003/0x3003) right up through
			 * "before efi_init", then as 0x80400000ffc00073-style
			 * entries (physical frame 0xffc00 = -4MB mod 2^32)
			 * immediately "after efi_init".
			 *
			 * First attempt: set VirtualStart = PhysicalStart (an
			 * identity mapping) so every region gets a distinct virtual
			 * address. That stopped the corruption, but pmap_map_bd()
			 * then panicked with "Invalid kernel address" -- it doesn't
			 * create new page table pages, only writes into ones that
			 * already exist, and OVMF's runtime-services region sits
			 * near 4GB, far outside our kernel's tiny low identity map
			 * ([0, physfree), tens of MB). Real Mac firmware's runtime
			 * regions apparently stay low enough for this not to matter;
			 * QEMU/OVMF's don't.
			 *
			 * Simplest correct fix given this kernel never calls an EFI
			 * runtime service: strip EFI_MEMORY_RUNTIME from every
			 * descriptor before handing off the memory map, so
			 * efi_init()'s mapping loop -- which only processes
			 * descriptors with that bit set -- skips all of them. */
#define EFI_MEMORY_RUNTIME 0x8000000000000000ULL
			/* boot_args/device-tree/memory-map buffer no longer need any
			 * EFI attribute/type protection here -- they're now folded
			 * into ba->ksize (see the g_low_alloc_next comment above and
			 * the ba->ksize assignment), so xnu treats them as part of
			 * its own kernel image and never reuses them regardless of
			 * what this descriptor's Type/Attribute say. */
			uint64_t total = 0;
			uint8_t *p = (uint8_t *)(UINTN)mmap_phys;
			for (UINTN off = 0; off < mmap_size; off += desc_size) {
				EFI_MEMORY_DESCRIPTOR *d = (EFI_MEMORY_DESCRIPTOR *)(p + off);
				if (d->Type != EfiMemoryMappedIO && d->Type != EfiMemoryMappedIOPortSpace) {
					total += d->NumberOfPages * PAGE_SIZE;
				}
				d->Attribute &= ~EFI_MEMORY_RUNTIME;
			}
			ba->PhysicalMemorySize = total;
		}
		serial_puts("[diag] PhysicalMemorySize=");
		serial_puthex64(ba->PhysicalMemorySize);
		serial_puts(" K64_MAXMEM=");
		serial_puthex64(1536ULL * 1024 * 1024 * 1024);
		serial_puts("\n");

		serial_puts("[boot] calling ExitBootServices\n");
		status = gBS->ExitBootServices(ImageHandle, map_key);
		if (!EFI_ERROR(status)) {
			break;
		}
		serial_puts("[boot] ExitBootServices failed, retrying\n");
	}

	if (EFI_ERROR(status)) {
		serial_puts("[boot] ExitBootServices gave up\n");
		return EFI_LOAD_ERROR;
	}

	serial_puts("[boot] jumping to kernel entry=");
	serial_puthex64(entry_phys);
	serial_puts(" boot_args=");
	serial_puthex64((uint64_t)ba_phys);
	serial_puts("\n");

	jump_to_kernel(entry_phys, (uint32_t)ba_phys);

	/* never reached */
	for (;;) {
	}
	return EFI_SUCCESS;
}
