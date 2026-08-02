#ifndef _UUID_UUID_H_
#define _UUID_UUID_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/_types/_uuid_t.h>
typedef char uuid_string_t[37];

void uuid_generate(uuid_t out);
void uuid_generate_random(uuid_t out);
void uuid_unparse(const uuid_t uu, uuid_string_t out);
void uuid_unparse_lower(const uuid_t uu, uuid_string_t out);
void uuid_unparse_upper(const uuid_t uu, uuid_string_t out);

#ifdef __cplusplus
}
#endif

#endif /* _UUID_UUID_H_ */
