/* Stub -- no networking; see netdb.h. */
#ifndef _SYS_SOCKET_H_
#define _SYS_SOCKET_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

typedef unsigned int socklen_t;
typedef unsigned short sa_family_t;

struct sockaddr {
	unsigned char sa_len;
	sa_family_t   sa_family;
	char          sa_data[14];
};

struct sockaddr_storage {
	unsigned char ss_len;
	sa_family_t   ss_family;
	char          ss_pad[126];
};

#define AF_UNSPEC 0
#define AF_UNIX   1
#define AF_LOCAL  AF_UNIX
#define AF_INET   2
#define AF_INET6  30
#define SOCK_STREAM    1
#define SOCK_DGRAM     2
#define SOCK_RAW       3
#define SOCK_RDM       4
#define SOCK_SEQPACKET 5

#define SHUT_RD   0
#define SHUT_WR   1
#define SHUT_RDWR 2

#define SOL_SOCKET    0xffff
#define SO_REUSEADDR  0x0004
#define SO_BROADCAST  0x0020
#define SO_KEEPALIVE  0x0008
#define SO_ERROR      0x1007

int socket(int domain, int type, int protocol);
int bind(int s, const struct sockaddr *addr, socklen_t len);
int connect(int s, const struct sockaddr *addr, socklen_t len);
int listen(int s, int backlog);
int accept(int s, struct sockaddr *addr, socklen_t *len);
int getsockname(int s, struct sockaddr *addr, socklen_t *len);
int getpeername(int s, struct sockaddr *addr, socklen_t *len);
ssize_t send(int s, const void *buf, size_t len, int flags);
ssize_t sendto(int s, const void *buf, size_t len, int flags, const struct sockaddr *to, socklen_t tolen);
ssize_t recvfrom(int s, void *buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen);
ssize_t recv(int s, void *buf, size_t len, int flags);
int setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen);
int getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen);
int shutdown(int s, int how);

#ifdef __cplusplus
}
#endif

#endif /* _SYS_SOCKET_H_ */
