/* No network stack in this kernel config (see docs/architecture.md) --
 * every enabled busybox applet is local-only, but a few shared libbb
 * helper files (bb_getsockname.c etc.) unconditionally reference the
 * BSD sockets API at compile time even though nothing in our .config
 * ever calls into them at runtime. Stub implementations that report
 * failure keep the link satisfied without pretending to have working
 * networking. */
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>

extern int errno;
int h_errno;
const char *hstrerror(int err) { (void)err; return "network not supported"; }

int socket(int domain, int type, int protocol) { (void)domain; (void)type; (void)protocol; errno = ENOSYS; return -1; }
int bind(int s, const struct sockaddr *addr, socklen_t len) { (void)s; (void)addr; (void)len; errno = ENOSYS; return -1; }
int connect(int s, const struct sockaddr *addr, socklen_t len) { (void)s; (void)addr; (void)len; errno = ENOSYS; return -1; }
int listen(int s, int backlog) { (void)s; (void)backlog; errno = ENOSYS; return -1; }
int accept(int s, struct sockaddr *addr, socklen_t *len) { (void)s; (void)addr; (void)len; errno = ENOSYS; return -1; }
ssize_t send(int s, const void *buf, size_t len, int flags) { (void)s; (void)buf; (void)len; (void)flags; errno = ENOSYS; return -1; }
ssize_t sendto(int s, const void *buf, size_t len, int flags, const struct sockaddr *to, socklen_t tolen) { (void)s; (void)buf; (void)len; (void)flags; (void)to; (void)tolen; errno = ENOSYS; return -1; }
ssize_t recvfrom(int s, void *buf, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen) { (void)s; (void)buf; (void)len; (void)flags; (void)from; (void)fromlen; errno = ENOSYS; return -1; }
ssize_t recv(int s, void *buf, size_t len, int flags) { (void)s; (void)buf; (void)len; (void)flags; errno = ENOSYS; return -1; }
int setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen) { (void)s; (void)level; (void)optname; (void)optval; (void)optlen; errno = ENOSYS; return -1; }
int getsockopt(int s, int level, int optname, void *optval, socklen_t *optlen) { (void)s; (void)level; (void)optname; (void)optval; (void)optlen; errno = ENOSYS; return -1; }
int shutdown(int s, int how) { (void)s; (void)how; errno = ENOSYS; return -1; }
int getsockname(int s, struct sockaddr *addr, socklen_t *len) { (void)s; (void)addr; (void)len; errno = ENOSYS; return -1; }
int getpeername(int s, struct sockaddr *addr, socklen_t *len) { (void)s; (void)addr; (void)len; errno = ENOSYS; return -1; }

struct hostent *gethostbyname(const char *name) { (void)name; return (void *)0; }
struct hostent *gethostbyaddr(const void *addr, size_t len, int type) { (void)addr; (void)len; (void)type; return (void *)0; }
int getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res)
{
	(void)node; (void)service; (void)hints; (void)res;
	return -1;
}
void freeaddrinfo(struct addrinfo *res) { (void)res; }
const char *gai_strerror(int errcode) { (void)errcode; return "network not supported"; }
struct servent *getservbyname(const char *name, const char *proto) { (void)name; (void)proto; return (void *)0; }

const char *inet_ntop(int af, const void *src, char *dst, unsigned int size) { (void)af; (void)src; (void)dst; (void)size; return (void *)0; }
int inet_pton(int af, const char *src, void *dst) { (void)af; (void)src; (void)dst; return -1; }
in_addr_t inet_addr(const char *cp) { (void)cp; return (in_addr_t)-1; }
int inet_aton(const char *cp, struct in_addr *addr) { (void)cp; (void)addr; return 0; }
char *inet_ntoa(struct in_addr in) { (void)in; static char buf[16] = "0.0.0.0"; return buf; }

int getnameinfo(const struct sockaddr *sa, socklen_t salen, char *host, size_t hostlen,
    char *serv, size_t servlen, int flags)
{
	(void)sa; (void)salen; (void)flags;
	if (host && hostlen) { host[0] = 0; }
	if (serv && servlen) { serv[0] = 0; }
	return -1;
}
