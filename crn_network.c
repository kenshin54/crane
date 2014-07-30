#include "crane.h"
#include "crn_network.h"
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>

int
crn_set_flag(char *name, int flag) {
	int sockfd;
	struct ifreq ifr;
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	if (sockfd < 0)
		return -1;

	memset(&ifr, 0, sizeof(ifr));

	strncpy(ifr.ifr_name, name, IFNAMSIZ);

	ifr.ifr_flags |= flag;
	return ioctl(sockfd, SIOCSIFFLAGS, &ifr);
}

int
crn_ifup(char *name) {
	return crn_set_flag(name, IFF_UP);
}

int
crn_ifdown(char *name) {
	return crn_set_flag(name, 0);
}
