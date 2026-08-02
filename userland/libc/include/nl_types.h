/* Message catalogs (gencat/catopen/catgets) require an installed
 * catalog file format and locale infrastructure this environment
 * doesn't have -- see locale.h. catopen() honestly reports "no such
 * catalog" rather than pretending to load one. */
#ifndef _NL_TYPES_H_
#define _NL_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

#define NL_SETD      1
#define NL_CAT_LOCALE 1

typedef void *nl_catd;

nl_catd catopen(const char *name, int oflag);
char *catgets(nl_catd catd, int set_id, int msg_id, const char *s);
int catclose(nl_catd catd);

#ifdef __cplusplus
}
#endif

#endif /* _NL_TYPES_H_ */
