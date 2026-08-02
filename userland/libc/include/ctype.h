#ifndef _CTYPE_H_
#define _CTYPE_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Real Darwin's ctype.h macros are themselves implemented via
 * _DefaultRuneLocale table lookups, so it transitively brings in
 * runetype.h -- libc++'s locale.cpp relies on exactly that chain to
 * see _DefaultRuneLocale without including runetype.h itself. */
#include <runetype.h>

/* Real Darwin's internal per-character classification bitmask values
 * (ground-truthed against sys/_ctype.h) -- libc++'s <locale> hardcodes
 * std::ctype_base::mask to these exact values on Apple targets. Only
 * the values need to match; nothing here does a real Darwin rune-table
 * lookup with them. */
#define _CTYPE_A  0x00000100L
#define _CTYPE_C  0x00000200L
#define _CTYPE_D  0x00000400L
#define _CTYPE_G  0x00000800L
#define _CTYPE_L  0x00001000L
#define _CTYPE_P  0x00002000L
#define _CTYPE_S  0x00004000L
#define _CTYPE_U  0x00008000L
#define _CTYPE_X  0x00010000L
#define _CTYPE_B  0x00020000L
#define _CTYPE_R  0x00040000L
#define _CTYPE_I  0x00080000L
#define _CTYPE_T  0x00100000L
#define _CTYPE_Q  0x00200000L

static inline int isdigit(int c) { return c >= '0' && c <= '9'; }
static inline int isxdigit(int c) { return isdigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }
static inline int islower(int c) { return c >= 'a' && c <= 'z'; }
static inline int isupper(int c) { return c >= 'A' && c <= 'Z'; }
static inline int isalpha(int c) { return islower(c) || isupper(c); }
static inline int isalnum(int c) { return isalpha(c) || isdigit(c); }
static inline int isspace(int c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f'; }
static inline int isblank(int c) { return c == ' ' || c == '\t'; }
static inline int iscntrl(int c) { return (c >= 0 && c < 0x20) || c == 0x7f; }
static inline int isprint(int c) { return c >= 0x20 && c < 0x7f; }
static inline int isgraph(int c) { return isprint(c) && c != ' '; }
static inline int ispunct(int c) { return isprint(c) && !isalnum(c) && c != ' '; }
static inline int isascii(int c) { return c >= 0 && c < 0x80; }
static inline int toupper(int c) { return islower(c) ? c - 'a' + 'A' : c; }
static inline int tolower(int c) { return isupper(c) ? c - 'A' + 'a' : c; }
static inline int toascii(int c) { return c & 0x7f; }

#ifdef __cplusplus
}
#endif

#endif /* _CTYPE_H_ */
