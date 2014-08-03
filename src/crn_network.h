#ifndef CRN_NETWORK_H
#define CRN_NETWORK_H

#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <linux/sockios.h>
#include "nl.h"

typedef struct {
	char *bridge;
	char *ip;
	char *gateway;
	char *broadcast;
	unsigned short mask;
} crn_network_t;

int crn_ifup(char *name);
int crn_ifdown(char *name);
int crn_create_veth_pair(char *name1, char *name2);

int lxc_ipv4_addr_add(int ifindex, struct in_addr *addr,
		      struct in_addr *bcast, int prefix);

int lxc_ipv4_gateway_add(int ifindex, struct in_addr *gw);

int lxc_bridge_attach(const char *bridge, const char *ifname);

int lxc_netdev_delete_by_index(int ifindex);
int lxc_netdev_delete_by_name(const char *name);

crn_network_t *crn_create_network(char *network, char *bridge);
void crn_free_network(crn_network_t *network);

#endif
