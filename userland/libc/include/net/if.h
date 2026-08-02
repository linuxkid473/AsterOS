/* Stub -- no networking; see netdb.h. */
#ifndef _NET_IF_H_
#define _NET_IF_H_

#ifdef __cplusplus
extern "C" {
#endif

#define IFNAMSIZ 16

struct ifreq {
	char ifr_name[IFNAMSIZ];
	union {
		char ifr_data[16];
	} ifr_ifru;
};

struct if_nameindex {
	unsigned int if_index;
	char *if_name;
};

#ifdef __cplusplus
}
#endif

#endif /* _NET_IF_H_ */
