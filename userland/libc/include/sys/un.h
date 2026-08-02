/* Stub -- no networking; see netdb.h. */
#ifndef _SYS_UN_H_
#define _SYS_UN_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/socket.h>

struct sockaddr_un {
	unsigned char sun_len;
	sa_family_t   sun_family;
	char          sun_path[104];
};

#ifdef __cplusplus
}
#endif

#endif /* _SYS_UN_H_ */
