/* No APFS clonefile()/copy-on-write optimization in this filesystem
 * (fat16lite) -- copyfile() here is a real, correct data copy (read
 * every byte of the source, write it to the destination), just not the
 * zero-copy fast path real Darwin's APFS gets. Only COPYFILE_DATA is
 * implemented since that's the only flag combination LLVM actually
 * uses (see llvm/lib/Support/Unix/Path.inc). */
#ifndef _COPYFILE_H_
#define _COPYFILE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef void *copyfile_state_t;
typedef unsigned int copyfile_flags_t;

#define COPYFILE_DATA        (1 << 3)
#define COPYFILE_METADATA    0
#define COPYFILE_ALL         (COPYFILE_METADATA | COPYFILE_DATA)
#define COPYFILE_CLONE       (1 << 24)
#define COPYFILE_CLONE_FORCE (1 << 25)
#define COPYFILE_DATA_SPARSE (1 << 27)

int copyfile(const char *from, const char *to, copyfile_state_t state, copyfile_flags_t flags);
int fcopyfile(int from_fd, int to_fd, copyfile_state_t state, copyfile_flags_t flags);

/* Our copyfile()/fcopyfile() never inspect state's contents (see
 * copyfile.c) -- it exists only so callers that allocate/free one
 * around the call (matching real Darwin's API contract) have
 * something valid to hold. */
copyfile_state_t copyfile_state_alloc(void);
int copyfile_state_free(copyfile_state_t state);

#ifdef __cplusplus
}
#endif

#endif /* _COPYFILE_H_ */
