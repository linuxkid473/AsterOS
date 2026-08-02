/* Stub -- no networking; see netdb.h. */
#ifndef _NETINET_IN_H_
#define _NETINET_IN_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/socket.h>
#include <sys/_endian.h>
#include <stdint.h>

typedef uint32_t in_addr_t;
typedef uint16_t in_port_t;

struct in_addr {
	in_addr_t s_addr;
};

struct sockaddr_in {
	unsigned char  sin_len;
	sa_family_t    sin_family;
	in_port_t      sin_port;
	struct in_addr sin_addr;
	char           sin_zero[8];
};

struct in6_addr {
	unsigned char s6_addr[16];
};

struct sockaddr_in6 {
	unsigned char   sin6_len;
	sa_family_t     sin6_family;
	in_port_t       sin6_port;
	uint32_t        sin6_flowinfo;
	struct in6_addr sin6_addr;
	uint32_t        sin6_scope_id;
};

#define INADDR_ANY ((in_addr_t)0)
#define IPPROTO_IP  0
#define IPPROTO_TCP 6
#define IPPROTO_UDP 17

/* htons/ntohs/htonl/ntohl come from the #include <sys/_endian.h> above --
 * this used to redefine them here as static inline functions, which
 * collided (redefinition errors) whenever some other header in the same
 * translation unit pulled in sys/_endian.h's macro versions first (e.g.
 * busybox's libbb.h: pwd.h/grp.h -> machine/endian.h -> sys/_endian.h,
 * ahead of its own <netinet/in.h> include). sys/_endian.h is the real,
 * ground-truthed Darwin header for these; this file has no business
 * defining its own second copy. */

/* Real Apple SDK headers pull the arpa/inet.h declarations in
 * transitively from here -- busybox's include/libbb.h relies on exactly
 * that under __APPLE__ (only includes <netinet/in.h>, not
 * <arpa/inet.h>), so we mirror it rather than patch busybox. */
const char *inet_ntop(int af, const void *src, char *dst, unsigned int size);
int inet_pton(int af, const char *src, void *dst);
in_addr_t inet_addr(const char *cp);
int inet_aton(const char *cp, struct in_addr *addr);
char *inet_ntoa(struct in_addr in);

#ifdef __cplusplus
}
#endif

#endif /* _NETINET_IN_H_ */
