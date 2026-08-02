/*
 * AsterOS's tiny init/shell -- exec'd directly by the kernel
 * as PID 1 (mockfs exposes our RAMDisk content as /sbin/launchd, which is
 * one of the hardcoded init candidate paths in bsd/kern/kern_exec.c). No
 * fork/exec of external programs: every command below is a builtin
 * (documented deviation from literal BusyBox, see docs/architecture.md),
 * since mockfs's root device *is* this single executable -- there is
 * nothing else to exec.
 *
 * Filesystem model (see docs/architecture.md "Decision: root filesystem"):
 *   - /dev is a REAL devfs mount (we mount it ourselves at startup) --
 *     ls/cd/cat/rm under /dev use real syscalls against real device nodes.
 *   - Everything else is an in-memory tree we own entirely (mockfs itself
 *     is read-only and only knows about /sbin, /dev, and the launchd file,
 *     so there is no other real writable storage to speak of) -- mkdir,
 *     rm, and echo-redirection operate on this tree via a static arena
 *     allocator (no mmap/brk syscalls needed).
 */
#include "syscall.h"
#include "mini_string.h"
#include "console.h"

#define PATH_MAX_LOCAL 256
#define MAX_NODES      256
#define ARENA_SIZE     (256 * 1024)
#define MAX_ARGV       32
#define LINE_MAX_LOCAL 512

/* ---- static arena allocator (no mmap/brk -- see file header) ---- */
static unsigned char g_arena[ARENA_SIZE];
static size_t g_arena_next;

static void *
arena_alloc(size_t n)
{
	n = (n + 7) & ~(size_t)7; /* keep everything 8-byte aligned */
	if (g_arena_next + n > ARENA_SIZE) {
		return 0;
	}
	void *p = &g_arena[g_arena_next];
	g_arena_next += n;
	return p;
}

/* ---- in-memory virtual filesystem tree ---- */
struct vnode {
	char           name[64];
	int            is_dir;
	struct vnode  *parent;
	struct vnode  *first_child;
	struct vnode  *next_sibling;
	char          *data;   /* file content, NULL for directories */
	size_t         size;   /* bytes of valid content */
};

static struct vnode g_node_pool[MAX_NODES];
static int          g_node_count;
static struct vnode *g_root;

static struct vnode *
node_alloc(const char *name, int is_dir, struct vnode *parent)
{
	if (g_node_count >= MAX_NODES) {
		return 0;
	}
	struct vnode *n = &g_node_pool[g_node_count++];
	xstrcpy(n->name, name, sizeof(n->name));
	n->is_dir = is_dir;
	n->parent = parent;
	n->first_child = 0;
	n->next_sibling = 0;
	n->data = 0;
	n->size = 0;
	if (parent) {
		n->next_sibling = parent->first_child;
		parent->first_child = n;
	}
	return n;
}

static void
vfs_init(void)
{
	g_node_count = 0;
	g_arena_next = 0;
	g_root = node_alloc("/", 1, 0);
}

static struct vnode *
find_child(struct vnode *dir, const char *name)
{
	for (struct vnode *c = dir->first_child; c; c = c->next_sibling) {
		if (xstrcmp(c->name, name) == 0) {
			return c;
		}
	}
	return 0;
}

/* ---- path handling ---- */
static int
is_dev_path(const char *path)
{
	return xstrncmp(path, "/dev", 4) == 0 && (path[4] == 0 || path[4] == '/');
}

/* Resolves `input` (absolute or relative to `cwd`) into a normalized
 * absolute path in `out` -- collapses ".", "..", and repeated slashes.
 * Pure string manipulation, no filesystem access. */
static void
normalize_path(const char *cwd, const char *input, char *out, size_t outcap)
{
	char tmp[PATH_MAX_LOCAL];

	if (input[0] == '/') {
		xstrcpy(tmp, input, sizeof(tmp));
	} else {
		xstrcpy(tmp, cwd, sizeof(tmp));
		if (xstrlen(tmp) == 0 || tmp[xstrlen(tmp) - 1] != '/') {
			xstrcat(tmp, "/", sizeof(tmp));
		}
		xstrcat(tmp, input, sizeof(tmp));
	}

	char *stack[64];
	int depth = 0;
	char *start = tmp;
	for (char *p = tmp;; p++) {
		if (*p == '/' || *p == 0) {
			int done = (*p == 0);
			*p = 0;
			if (start != p) {
				if (xstrcmp(start, ".") == 0) {
					/* no-op */
				} else if (xstrcmp(start, "..") == 0) {
					if (depth > 0) {
						depth--;
					}
				} else if (depth < 64) {
					stack[depth++] = start;
				}
			}
			start = p + 1;
			if (done) {
				break;
			}
		}
	}

	out[0] = '/';
	out[1] = 0;
	for (int i = 0; i < depth; i++) {
		if (i > 0) {
			xstrcat(out, "/", outcap);
		}
		xstrcat(out, stack[i], outcap);
	}
}

/* Walks the virtual tree for `path` (already normalized, absolute).
 * Returns the node, or NULL if any component is missing. If
 * `want_parent_of_last` is set, stops one component short and returns the
 * parent directory instead (used by mkdir/create to find where to attach a
 * new node), leaving the final component's name in `last_out`. */
static struct vnode *
vfs_walk(const char *path, int want_parent_of_last, char *last_out, size_t last_out_cap)
{
	if (xstrcmp(path, "/") == 0) {
		return want_parent_of_last ? 0 : g_root;
	}

	char tmp[PATH_MAX_LOCAL];
	xstrcpy(tmp, path + 1, sizeof(tmp)); /* skip leading '/' */

	struct vnode *cur = g_root;
	char *start = tmp;
	for (char *p = tmp;; p++) {
		if (*p == '/' || *p == 0) {
			int done = (*p == 0);
			*p = 0;
			int is_last = done;

			if (is_last && want_parent_of_last) {
				if (last_out) {
					xstrcpy(last_out, start, last_out_cap);
				}
				return cur;
			}

			if (!cur->is_dir) {
				return 0;
			}
			struct vnode *next = find_child(cur, start);
			if (!next) {
				return 0;
			}
			cur = next;

			start = p + 1;
			if (done) {
				break;
			}
		}
	}
	return cur;
}

static struct vnode *
vfs_lookup(const char *path)
{
	return vfs_walk(path, 0, 0, 0);
}

/* ---- global shell state ---- */
static char g_cwd[PATH_MAX_LOCAL] = "/";

/* ---- real (/dev) filesystem helpers ---- */
struct real_dirent64 {
	unsigned long long d_ino;
	unsigned long long d_seekoff;
	unsigned short      d_reclen;
	unsigned short      d_namlen;
	unsigned char       d_type;
	char                d_name[1024];
};

static void
real_ls(const char *path)
{
	int fd = sys_open(path, O_RDONLY, 0);
	if (fd < 0) {
		con_puts("ls: cannot access ");
		con_puts(path);
		con_puts("\n");
		return;
	}
	static unsigned char buf[8192];
	off_t pos = 0;
	for (;;) {
		ssize_t n = sys_getdirentries64(fd, buf, sizeof(buf), &pos);
		if (n <= 0) {
			break;
		}
		size_t off = 0;
		while (off < (size_t)n) {
			struct real_dirent64 *de = (struct real_dirent64 *)&buf[off];
			if (de->d_reclen == 0) {
				break;
			}
			if (!(de->d_namlen == 1 && de->d_name[0] == '.') &&
			    !(de->d_namlen == 2 && de->d_name[0] == '.' && de->d_name[1] == '.')) {
				con_puts(de->d_name);
				con_puts("  ");
			}
			off += de->d_reclen;
		}
	}
	con_puts("\n");
	sys_close(fd);
}

static void
real_cat(const char *path)
{
	int fd = sys_open(path, O_RDONLY, 0);
	if (fd < 0) {
		con_puts("cat: cannot open ");
		con_puts(path);
		con_puts("\n");
		return;
	}
	char buf[512];
	ssize_t n;
	while ((n = sys_read(fd, buf, sizeof(buf))) > 0) {
		con_write(buf, (size_t)n);
	}
	sys_close(fd);
}

/* ---- command tokenizer ---- */
static int
tokenize(char *line, char *argv[], int max_argv)
{
	int argc = 0;
	char *p = line;
	while (*p && argc < max_argv) {
		while (*p == ' ' || *p == '\t') {
			p++;
		}
		if (!*p) {
			break;
		}
		argv[argc++] = p;
		while (*p && *p != ' ' && *p != '\t') {
			p++;
		}
		if (*p) {
			*p++ = 0;
		}
	}
	return argc;
}

/* ---- builtins ---- */
static void
cmd_pwd(int argc, char **argv)
{
	(void)argc; (void)argv;
	con_puts(g_cwd);
	con_puts("\n");
}

static void
cmd_cd(int argc, char **argv)
{
	const char *target = (argc > 1) ? argv[1] : "/";
	char path[PATH_MAX_LOCAL];
	normalize_path(g_cwd, target, path, sizeof(path));

	if (is_dev_path(path)) {
		int fd = sys_open(path, O_RDONLY, 0);
		if (fd < 0) {
			con_puts("cd: no such directory: ");
			con_puts(path);
			con_puts("\n");
			return;
		}
		sys_close(fd);
		xstrcpy(g_cwd, path, sizeof(g_cwd));
		return;
	}

	struct vnode *n = vfs_lookup(path);
	if (!n || !n->is_dir) {
		con_puts("cd: no such directory: ");
		con_puts(path);
		con_puts("\n");
		return;
	}
	xstrcpy(g_cwd, path, sizeof(g_cwd));
}

static void
cmd_ls(int argc, char **argv)
{
	const char *target = (argc > 1) ? argv[1] : g_cwd;
	char path[PATH_MAX_LOCAL];
	normalize_path(g_cwd, target, path, sizeof(path));

	if (is_dev_path(path)) {
		real_ls(path);
		return;
	}

	struct vnode *n = vfs_lookup(path);
	if (!n) {
		con_puts("ls: no such file or directory: ");
		con_puts(path);
		con_puts("\n");
		return;
	}
	if (!n->is_dir) {
		con_puts(n->name);
		con_puts("\n");
		return;
	}
	for (struct vnode *c = n->first_child; c; c = c->next_sibling) {
		con_puts(c->name);
		if (c->is_dir) {
			con_puts("/");
		}
		con_puts("  ");
	}
	con_puts("\n");
}

static void
cmd_cat(int argc, char **argv)
{
	if (argc < 2) {
		con_puts("usage: cat <path> [path...]\n");
		return;
	}
	for (int i = 1; i < argc; i++) {
		char path[PATH_MAX_LOCAL];
		normalize_path(g_cwd, argv[i], path, sizeof(path));

		if (is_dev_path(path)) {
			real_cat(path);
			continue;
		}

		struct vnode *n = vfs_lookup(path);
		if (!n) {
			con_puts("cat: no such file: ");
			con_puts(path);
			con_puts("\n");
			continue;
		}
		if (n->is_dir) {
			con_puts("cat: is a directory: ");
			con_puts(path);
			con_puts("\n");
			continue;
		}
		if (n->data && n->size) {
			con_write(n->data, n->size);
		}
	}
}

static void
cmd_echo(int argc, char **argv)
{
	int redirect_at = -1;
	for (int i = 1; i < argc; i++) {
		if (xstrcmp(argv[i], ">") == 0) {
			redirect_at = i;
			break;
		}
	}

	int last = (redirect_at >= 0) ? redirect_at : argc;

	if (redirect_at < 0) {
		for (int i = 1; i < last; i++) {
			if (i > 1) {
				con_puts(" ");
			}
			con_puts(argv[i]);
		}
		con_puts("\n");
		return;
	}

	if (redirect_at + 1 >= argc) {
		con_puts("echo: missing redirection target\n");
		return;
	}

	char buf[LINE_MAX_LOCAL];
	buf[0] = 0;
	for (int i = 1; i < last; i++) {
		if (i > 1) {
			xstrcat(buf, " ", sizeof(buf));
		}
		xstrcat(buf, argv[i], sizeof(buf));
	}
	xstrcat(buf, "\n", sizeof(buf));

	char path[PATH_MAX_LOCAL];
	normalize_path(g_cwd, argv[redirect_at + 1], path, sizeof(path));

	if (is_dev_path(path)) {
		con_puts("echo: cannot write under /dev (real device nodes only)\n");
		return;
	}

	char last_name[64];
	struct vnode *parent = vfs_walk(path, 1, last_name, sizeof(last_name));
	if (!parent || !parent->is_dir) {
		con_puts("echo: no such directory for: ");
		con_puts(path);
		con_puts("\n");
		return;
	}
	struct vnode *file = find_child(parent, last_name);
	if (!file) {
		file = node_alloc(last_name, 0, parent);
		if (!file) {
			con_puts("echo: out of node space\n");
			return;
		}
	}
	if (file->is_dir) {
		con_puts("echo: is a directory: ");
		con_puts(path);
		con_puts("\n");
		return;
	}
	size_t len = xstrlen(buf);
	char *storage = (char *)arena_alloc(len);
	if (!storage) {
		con_puts("echo: out of storage space\n");
		return;
	}
	xmemcpy(storage, buf, len);
	file->data = storage;
	file->size = len;
}

static void
cmd_mkdir(int argc, char **argv)
{
	if (argc < 2) {
		con_puts("usage: mkdir <path>\n");
		return;
	}
	char path[PATH_MAX_LOCAL];
	normalize_path(g_cwd, argv[1], path, sizeof(path));

	if (is_dev_path(path)) {
		con_puts("mkdir: read-only filesystem: ");
		con_puts(path);
		con_puts("\n");
		return;
	}

	char last_name[64];
	struct vnode *parent = vfs_walk(path, 1, last_name, sizeof(last_name));
	if (!parent || !parent->is_dir) {
		con_puts("mkdir: no such directory: ");
		con_puts(path);
		con_puts("\n");
		return;
	}
	if (find_child(parent, last_name)) {
		con_puts("mkdir: already exists: ");
		con_puts(path);
		con_puts("\n");
		return;
	}
	if (!node_alloc(last_name, 1, parent)) {
		con_puts("mkdir: out of node space\n");
	}
}

static void
cmd_rm(int argc, char **argv)
{
	if (argc < 2) {
		con_puts("usage: rm <path>\n");
		return;
	}
	char path[PATH_MAX_LOCAL];
	normalize_path(g_cwd, argv[1], path, sizeof(path));

	if (is_dev_path(path)) {
		/* Let the kernel report the real error (EROFS/EPERM) --
		 * devfs genuinely is real, so this is a real syscall, not a
		 * simulated one. */
		if (sys_unlink(path) < 0) {
			con_puts("rm: cannot remove: ");
			con_puts(path);
			con_puts("\n");
		}
		return;
	}

	char last_name[64];
	struct vnode *parent = vfs_walk(path, 1, last_name, sizeof(last_name));
	if (!parent) {
		con_puts("rm: no such file: ");
		con_puts(path);
		con_puts("\n");
		return;
	}
	struct vnode *target = find_child(parent, last_name);
	if (!target) {
		con_puts("rm: no such file: ");
		con_puts(path);
		con_puts("\n");
		return;
	}
	if (target->is_dir && target->first_child) {
		con_puts("rm: directory not empty: ");
		con_puts(path);
		con_puts("\n");
		return;
	}
	/* Unlink from the parent's child list (single-linked -- walk to find
	 * the predecessor). Leaves the slot in g_node_pool allocated (this is
	 * a tiny init, not a long-running shell with heavy churn -- keeping
	 * the allocator a simple bump allocator is worth never reclaiming
	 * individual nodes). */
	if (parent->first_child == target) {
		parent->first_child = target->next_sibling;
	} else {
		struct vnode *prev = parent->first_child;
		while (prev && prev->next_sibling != target) {
			prev = prev->next_sibling;
		}
		if (prev) {
			prev->next_sibling = target->next_sibling;
		}
	}
}

static void
cmd_mount(int argc, char **argv)
{
	(void)argv;
	if (argc == 1) {
		if (sys_mount("devfs", "/dev", 0, 0) < 0) {
			con_puts("mount: devfs on /dev failed (already mounted?)\n");
		} else {
			con_puts("devfs on /dev\n");
		}
		return;
	}
	con_puts("mount: only bare 'mount' (devfs on /dev) is supported\n");
}

static void
cmd_uname(int argc, char **argv)
{
	int show_all = (argc > 1 && xstrcmp(argv[1], "-a") == 0);
	if (show_all) {
		con_puts("Darwin asteros 19.6.0 Darwin Kernel Version 19.6.0: "
		    "xnu-6153.141.1 (AsterOS) x86_64\n");
	} else {
		con_puts("Darwin\n");
	}
}

/* ---- dispatch + main loop ---- */
static void
dispatch(int argc, char **argv)
{
	const char *cmd = argv[0];

	if (xstrcmp(cmd, "ls") == 0) {
		cmd_ls(argc, argv);
	} else if (xstrcmp(cmd, "cd") == 0) {
		cmd_cd(argc, argv);
	} else if (xstrcmp(cmd, "pwd") == 0) {
		cmd_pwd(argc, argv);
	} else if (xstrcmp(cmd, "cat") == 0) {
		cmd_cat(argc, argv);
	} else if (xstrcmp(cmd, "echo") == 0) {
		cmd_echo(argc, argv);
	} else if (xstrcmp(cmd, "mkdir") == 0) {
		cmd_mkdir(argc, argv);
	} else if (xstrcmp(cmd, "rm") == 0) {
		cmd_rm(argc, argv);
	} else if (xstrcmp(cmd, "mount") == 0) {
		cmd_mount(argc, argv);
	} else if (xstrcmp(cmd, "uname") == 0) {
		cmd_uname(argc, argv);
	} else {
		con_puts(cmd);
		con_puts(": command not found\n");
	}
}

int
main(void)
{
	/* /dev is a real devfs mount; mockfs only gives us the empty
	 * mountpoint directory. Failure here (e.g. already mounted) is not
	 * fatal -- keep booting either way. */
	sys_mount("devfs", "/dev", 0, 0);

	/* We are PID 1, exec'd directly by the kernel with no pre-opened
	 * file descriptors (unlike a forked process, which would inherit
	 * its parent's fd table) -- fd 0/1/2 do not exist yet. Without
	 * this, sys_read(0,...)/sys_write(1,...) below both fail with
	 * EBADF (n<0), and con_readline's "n<0 -> continue" turns main's
	 * loop into a zero-output hot spin that looks identical to a
	 * kernel hang from the serial log. Opening /dev/console (created
	 * unconditionally by devfs_init(), bsd/miscfs/devfs/devfs_vfsops.c)
	 * three times in a row claims fd 0, 1, 2 in order, since open()
	 * always hands out the lowest free descriptor. */
	sys_open("/dev/console", O_RDWR, 0);
	sys_open("/dev/console", O_RDWR, 0);
	sys_open("/dev/console", O_RDWR, 0);

	vfs_init();
	xstrcpy(g_cwd, "/", sizeof(g_cwd));

	con_puts("AsterOS -- minimal init/shell\n");
	con_puts("commands: ls cd pwd cat echo mkdir rm mount uname\n\n");

	for (;;) {
		con_puts(g_cwd);
		con_puts(" # ");

		char line[LINE_MAX_LOCAL];
		int n = con_readline(line, sizeof(line));
		if (n < 0) {
			/* No console input source (e.g. read() failed outright) --
			 * don't spin a hot loop retrying forever. */
			continue;
		}

		char *argv[MAX_ARGV];
		int argc = tokenize(line, argv, MAX_ARGV);
		if (argc == 0) {
			continue;
		}
		dispatch(argc, argv);
	}
}
