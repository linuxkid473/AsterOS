/* Stub -- no networking in this environment. Present only so headers that
 * unconditionally #include <netdb.h> (e.g. busybox's libbb.h) parse; the
 * declarations here are never called since no network applet is enabled
 * in our .config. */
#ifndef _NETDB_H_
#define _NETDB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <sys/socket.h>

struct hostent {
	char  *h_name;
	char **h_aliases;
	int    h_addrtype;
	int    h_length;
	char **h_addr_list;
};
#define h_addr h_addr_list[0]

struct addrinfo {
	int              ai_flags;
	int              ai_family;
	int              ai_socktype;
	int              ai_protocol;
	unsigned int     ai_addrlen;
	struct sockaddr *ai_addr;
	char            *ai_canonname;
	struct addrinfo *ai_next;
};

struct servent {
	char  *s_name;
	char **s_aliases;
	int    s_port;
	char  *s_proto;
};

#define AI_PASSIVE     0x0001
#define AI_CANONNAME   0x0002
#define AI_NUMERICHOST 0x0004
#define AI_NUMERICSERV 0x1000

#define NI_NOFQDN       0x01
#define NI_NUMERICHOST  0x02
#define NI_NAMEREQD     0x04
#define NI_NUMERICSERV  0x08
#define NI_DGRAM        0x10
#define NI_NUMERICSCOPE 0x20
#define NI_MAXHOST      1025
#define NI_MAXSERV      32

int getnameinfo(const struct sockaddr *sa, socklen_t salen,
    char *host, size_t hostlen, char *serv, size_t servlen, int flags);

extern int h_errno;
const char *hstrerror(int err);

struct hostent *gethostbyname(const char *name);
struct hostent *gethostbyaddr(const void *addr, size_t len, int type);
int getaddrinfo(const char *node, const char *service,
    const struct addrinfo *hints, struct addrinfo **res);
void freeaddrinfo(struct addrinfo *res);
const char *gai_strerror(int errcode);
struct servent *getservbyname(const char *name, const char *proto);

#ifdef __cplusplus
}
#endif

#endif /* _NETDB_H_ */
