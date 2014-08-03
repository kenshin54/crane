#include "crane.h"
#include "nl.h"
#include "crn_network.h"
#include "crn_utils.h"

#ifndef VETH_INFO_PEER
# define VETH_INFO_PEER 1
#endif

#define NEXT_POS(p, next) (p = next + 1)

struct link_req {
	struct nlmsg nlmsg;
	struct ifinfomsg ifinfomsg;
};

struct ip_req {
	struct nlmsg nlmsg;
	struct ifaddrmsg ifa;
};

struct rt_req {
	struct nlmsg nlmsg;
	struct rtmsg rt;
};

int
crn_set_flag(char *name, int flag) {
	struct nl_handler nlh;
	struct nlmsg *nlmsg = NULL, *answer = NULL;
	struct link_req *link_req;
	int index, len, err;

	err = netlink_open(&nlh, NETLINK_ROUTE);
	if (err)
		return err;

	err = -EINVAL;
	len = strlen(name);
	if (len == 1 || len >= IFNAMSIZ)
		goto out;

	err = -ENOMEM;
	nlmsg = nlmsg_alloc(NLMSG_GOOD_SIZE);
	if (!nlmsg)
		goto out;

	answer = nlmsg_alloc(NLMSG_GOOD_SIZE);
	if (!answer)
		goto out;

	err = -EINVAL;
	index = if_nametoindex(name);
	if (!index)
		goto out;

	link_req = (struct link_req *)nlmsg;
	link_req->ifinfomsg.ifi_family = AF_UNSPEC;
	link_req->ifinfomsg.ifi_index = index;
	link_req->ifinfomsg.ifi_change |= IFF_UP;
	link_req->ifinfomsg.ifi_flags |= flag;
	nlmsg->nlmsghdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
	nlmsg->nlmsghdr.nlmsg_flags = NLM_F_REQUEST|NLM_F_ACK;
	nlmsg->nlmsghdr.nlmsg_type = RTM_NEWLINK;

	err = netlink_transaction(&nlh, nlmsg, answer);
out:
	netlink_close(&nlh);
	nlmsg_free(nlmsg);
	nlmsg_free(answer);
	return err;
}

int
crn_ifup(char *name) {
	return crn_set_flag(name, IFF_UP);
}

int
crn_ifdown(char *name) {
	return crn_set_flag(name, 0);
}

int
crn_create_veth_pair(char *name1, char *name2) {
	struct nl_handler nlh;
	struct nlmsg *nlmsg = NULL, *answer = NULL;
	struct link_req *link_req;
	struct rtattr *nest1, *nest2, *nest3;
	int len, err;

	err = netlink_open(&nlh, NETLINK_ROUTE);
	if (err)
		return err;

	err = -EINVAL;
	len = strlen(name1);
	if (len == 1 || len >= IFNAMSIZ)
		goto out;

	len = strlen(name2);
	if (len == 1 || len >= IFNAMSIZ)
		goto out;

	err = -ENOMEM;
	nlmsg = nlmsg_alloc(NLMSG_GOOD_SIZE);
	if (!nlmsg)
		goto out;

	answer = nlmsg_alloc(NLMSG_GOOD_SIZE);
	if (!answer)
		goto out;

	link_req = (struct link_req *)nlmsg;
	link_req->ifinfomsg.ifi_family = AF_UNSPEC;
	nlmsg->nlmsghdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
	nlmsg->nlmsghdr.nlmsg_flags =
		NLM_F_REQUEST|NLM_F_CREATE|NLM_F_EXCL|NLM_F_ACK;
	nlmsg->nlmsghdr.nlmsg_type = RTM_NEWLINK;

	err = -EINVAL;
	nest1 = nla_begin_nested(nlmsg, IFLA_LINKINFO);
	if (!nest1)
		goto out;

	if (nla_put_string(nlmsg, IFLA_INFO_KIND, "veth"))
		goto out;

	nest2 = nla_begin_nested(nlmsg, IFLA_INFO_DATA);
	if (!nest2)
		goto out;

	nest3 = nla_begin_nested(nlmsg, VETH_INFO_PEER);
	if (!nest3)
		goto out;

	nlmsg->nlmsghdr.nlmsg_len += sizeof(struct ifinfomsg);

	if (nla_put_string(nlmsg, IFLA_IFNAME, name2))
		goto out;

	nla_end_nested(nlmsg, nest3);

	nla_end_nested(nlmsg, nest2);

	nla_end_nested(nlmsg, nest1);

	if (nla_put_string(nlmsg, IFLA_IFNAME, name1))
		goto out;

	err = netlink_transaction(&nlh, nlmsg, answer);
out:
	netlink_close(&nlh);
	nlmsg_free(answer);
	nlmsg_free(nlmsg);
	return err;
}

static int
ip_addr_add(int family, int ifindex,
		       void *addr, void *bcast, void *acast, int prefix) {
	struct nl_handler nlh;
	struct nlmsg *nlmsg = NULL, *answer = NULL;
	struct ip_req *ip_req;
	int addrlen;
	int err;

	addrlen = family == AF_INET ? sizeof(struct in_addr) :
		sizeof(struct in6_addr);

	err = netlink_open(&nlh, NETLINK_ROUTE);
	if (err)
		return err;

	err = -ENOMEM;
	nlmsg = nlmsg_alloc(NLMSG_GOOD_SIZE);
	if (!nlmsg)
		goto out;

	answer = nlmsg_alloc(NLMSG_GOOD_SIZE);
	if (!answer)
		goto out;

	ip_req = (struct ip_req *)nlmsg;
        ip_req->nlmsg.nlmsghdr.nlmsg_len =
		NLMSG_LENGTH(sizeof(struct ifaddrmsg));
        ip_req->nlmsg.nlmsghdr.nlmsg_flags =
		NLM_F_ACK|NLM_F_REQUEST|NLM_F_CREATE|NLM_F_EXCL;
        ip_req->nlmsg.nlmsghdr.nlmsg_type = RTM_NEWADDR;
	ip_req->ifa.ifa_prefixlen = prefix;
        ip_req->ifa.ifa_index = ifindex;
        ip_req->ifa.ifa_family = family;
	ip_req->ifa.ifa_scope = 0;
	
	err = -EINVAL;
	if (nla_put_buffer(nlmsg, IFA_LOCAL, addr, addrlen))
		goto out;

	if (nla_put_buffer(nlmsg, IFA_ADDRESS, addr, addrlen))
		goto out;

	if (nla_put_buffer(nlmsg, IFA_BROADCAST, bcast, addrlen))
		goto out;

	/* TODO : multicast, anycast with ipv6 */
	err = -EPROTONOSUPPORT;
	if (family == AF_INET6 &&
	    (memcmp(bcast, &in6addr_any, sizeof(in6addr_any)) ||
	     memcmp(acast, &in6addr_any, sizeof(in6addr_any))))
		goto out;

	err = netlink_transaction(&nlh, nlmsg, answer);
out:
	netlink_close(&nlh);
	nlmsg_free(answer);
	nlmsg_free(nlmsg);
	return err;
}

int
lxc_ipv4_addr_add(int ifindex, struct in_addr *addr,
		      struct in_addr *bcast, int prefix) {
	return ip_addr_add(AF_INET, ifindex, addr, bcast, NULL, prefix);
}

static int
ip_gateway_add(int family, int ifindex, void *gw) {
	struct nl_handler nlh;
	struct nlmsg *nlmsg = NULL, *answer = NULL;
	struct rt_req *rt_req;
	int addrlen;
	int err;

	addrlen = family == AF_INET ? sizeof(struct in_addr) :
		sizeof(struct in6_addr);

	err = netlink_open(&nlh, NETLINK_ROUTE);
	if (err)
		return err;

	err = -ENOMEM;
	nlmsg = nlmsg_alloc(NLMSG_GOOD_SIZE);
	if (!nlmsg)
		goto out;

	answer = nlmsg_alloc(NLMSG_GOOD_SIZE);
	if (!answer)
		goto out;

	rt_req = (struct rt_req *)nlmsg;
	rt_req->nlmsg.nlmsghdr.nlmsg_len =
		NLMSG_LENGTH(sizeof(struct rtmsg));
	rt_req->nlmsg.nlmsghdr.nlmsg_flags =
		NLM_F_ACK|NLM_F_REQUEST|NLM_F_CREATE|NLM_F_EXCL;
	rt_req->nlmsg.nlmsghdr.nlmsg_type = RTM_NEWROUTE;
	rt_req->rt.rtm_family = family;
	rt_req->rt.rtm_table = RT_TABLE_MAIN;
	rt_req->rt.rtm_scope = RT_SCOPE_UNIVERSE;
	rt_req->rt.rtm_protocol = RTPROT_BOOT;
	rt_req->rt.rtm_type = RTN_UNICAST;
	/* "default" destination */
	rt_req->rt.rtm_dst_len = 0;

	err = -EINVAL;
	if (nla_put_buffer(nlmsg, RTA_GATEWAY, gw, addrlen))
		goto out;

	/* Adding the interface index enables the use of link-local
	 * addresses for the gateway */
	if (nla_put_u32(nlmsg, RTA_OIF, ifindex))
		goto out;

	err = netlink_transaction(&nlh, nlmsg, answer);
out:
	netlink_close(&nlh);
	nlmsg_free(answer);
	nlmsg_free(nlmsg);
	return err;
}

int
lxc_ipv4_gateway_add(int ifindex, struct in_addr *gw) {
	return ip_gateway_add(AF_INET, ifindex, gw);
}

int
lxc_netdev_delete_by_index(int ifindex) {
	struct nl_handler nlh;
	struct nlmsg *nlmsg = NULL, *answer = NULL;
	struct link_req *link_req;
	int err;

	err = netlink_open(&nlh, NETLINK_ROUTE);
	if (err)
		return err;

	err = -ENOMEM;
	nlmsg = nlmsg_alloc(NLMSG_GOOD_SIZE);
	if (!nlmsg)
		goto out;

	answer = nlmsg_alloc(NLMSG_GOOD_SIZE);
	if (!answer)
		goto out;

	link_req = (struct link_req *)nlmsg;
	link_req->ifinfomsg.ifi_family = AF_UNSPEC;
	link_req->ifinfomsg.ifi_index = ifindex;
	nlmsg->nlmsghdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
	nlmsg->nlmsghdr.nlmsg_flags = NLM_F_ACK|NLM_F_REQUEST;
	nlmsg->nlmsghdr.nlmsg_type = RTM_DELLINK;

	err = netlink_transaction(&nlh, nlmsg, answer);
out:
	netlink_close(&nlh);
	nlmsg_free(answer);
	nlmsg_free(nlmsg);
	return err;
}

int
lxc_netdev_delete_by_name(const char *name) {
	int index;

	index = if_nametoindex(name);
	if (!index)
		return -EINVAL;

	return lxc_netdev_delete_by_index(index);
}

int
lxc_bridge_attach(const char *bridge, const char *ifname) {
	int fd, index, err;
	struct ifreq ifr;

	if (strlen(ifname) >= IFNAMSIZ)
		return -EINVAL;

	index = if_nametoindex(ifname);
	if (!index)
		return -EINVAL;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		return -errno;

	strncpy(ifr.ifr_name, bridge, IFNAMSIZ-1);
	ifr.ifr_name[IFNAMSIZ-1] = '\0';
	ifr.ifr_ifindex = index;
	err = ioctl(fd, SIOCBRADDIF, &ifr);
	close(fd);
	if (err)
		err = -errno;

	return err;
}

crn_network_t *
crn_create_network(char *network, char *bridge) {
	char *pos = NULL, *tmp = NULL;
	crn_network_t *net = malloc(sizeof(crn_network_t));
	net->bridge = malloc(strlen(bridge) + 1);
	strcpy(net->bridge, bridge);

	if ((pos = strchr(network, '/')) == NULL || pos == network) {
		goto err;
	}
	net->ip = crn_strncpy(network, pos - network);
	NEXT_POS(network, pos);

	if ((pos = strchr(network, '@')) == NULL || pos == network) {
		goto err;
	}
	tmp = crn_strncpy(network, pos - network);
	net->mask = atoi(tmp);
	free(tmp);
	NEXT_POS(network, pos);

	if ((pos = strchr(network, '#')) == NULL || pos == network) {
		goto err;
	}
	net->gateway = crn_strncpy(network, pos - network);
	NEXT_POS(network, pos);

	if (strlen(network) == 0) {
		goto err;
	}
	net->broadcast = malloc(strlen(network) + 1);
	strcpy(net->broadcast, network);

	return net;
err:
	free(net);
	return NULL;
}

void
crn_free_network(crn_network_t *network) {
	if (network) {
		if (network->bridge) {
			free(network->bridge);
		}
		if (network->ip) {
			free(network->ip);
		}
		if (network->gateway) {
			free(network->gateway);
		}
		if (network->broadcast) {
			free(network->broadcast);
		}
	}
}

