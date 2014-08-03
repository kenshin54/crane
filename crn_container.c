#include "crane.h"
#include "crn_list.h"
#include "crn_container.h"
#include "crn_utils.h"
#include "crn_aufs.h"
#include "nl.h"
#include "crn_network.h"
#include "crn_cgroup.h"
#include <semaphore.h>

#define NETNS_VETH_HOST "vh"
#define NETNS_VETH_GUEST "vg"
#define NETNS_DIR "/var/run/netns"
#define ETH_LOOPBACK "lo"
#define HOSTS_FILE "/etc/hosts"

#define SEM_NAME "crn_contaier_sem"
#define SEM_OPEN_FLAG O_RDWR|O_CREAT
#define SEM_OPEN_MODE 00777
#define INIT_V    0

static sem_t *sem = NULL;


static struct crn_mount_entry {
	char *source;
	char *target;
	char *fs;
	unsigned long flags;
	void *data;
} mount_entries[] = {
	{ .source = "proc",		.target = "/proc",		.fs = "proc",	.flags = 0, .data = NULL },
	{ .source = "sysfs",	.target = "/sys",		.fs = "sysfs",	.flags = 0, .data = NULL },
	{ .source = "devpts",	.target = "/dev/pts",	.fs = "devpts",	.flags = 0, .data = NULL },
	{ .source = NULL,		.target = NULL,			.fs = NULL,		.flags = 0, .data = NULL }
};

static struct crn_dev_entry {
	char *pathname;
	mode_t mode;
	unsigned int major;
	unsigned int minor;
} dev_entries[] = {
	{ .pathname = "/dev/console",	.mode = S_IRUSR|S_IWUSR|S_IWGRP|S_IWOTH|S_IFCHR,					.major = 5,  .minor = 1 },
	{ .pathname = "/dev/null",		.mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH|S_IFCHR,	.major = 1,  .minor = 3 },
	{ .pathname = "/dev/zero",		.mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH|S_IFCHR,	.major = 1,  .minor = 5 },
	{ .pathname = "/dev/ptmx",		.mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH|S_IFCHR,	.major = 5,  .minor = 2 },
	{ .pathname = "/dev/tty",		.mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH|S_IFCHR,	.major = 5,  .minor = 0 },
	{ .pathname = "/dev/random",	.mode = S_IRUSR|S_IRGRP|S_IROTH|S_IFCHR,							.major = 1,  .minor = 8 },
	{ .pathname = "/dev/urandom",	.mode = S_IRUSR|S_IRGRP|S_IROTH|S_IFCHR,							.major = 1,  .minor = 9 },
	{ .pathname = NULL,				.mode = 0,															.major = 0,  .minor = 0 }
};

static struct crn_host_entry {
	char *ip;
	char *hostname;
} host_entries[] = {
	{ .ip = "127.0.0.1",	.hostname = "localhost" },
	{ .ip = "::1",			.hostname = "localhost ip6-localhost ip6-loopback" },
	{ .ip = "fe00::0",		.hostname = "ip6-localnet" },
	{ .ip = "ff00::0",		.hostname = "ip6-mcastprefix" },
	{ .ip = "ff02::1",		.hostname = "ip6-allnodes" },
	{ .ip = "ff02::2",		.hostname = "ip6-allrouters" },
	{ .ip = NULL,			.hostname = NULL }
};

crn_container*
crn_alloc_container(char *name, char *base, char *root) {
	crn_container* container = malloc(sizeof(crn_container));
	if (container == NULL) {
		return NULL;
	}
	container->name = name;
	container->base = base;
	container->root = root;
	container->diff_path = malloc(sizeof(name) + sizeof(root) + sizeof(CRANE_STORAGE_DIFF) + 1);
	sprintf(container->diff_path, "%s/%s/%s", root, CRANE_STORAGE_DIFF, name);
	container->mnt_path = malloc(sizeof(name) + sizeof(root) + sizeof(CRANE_STORAGE_MNT) + 1);
	sprintf(container->mnt_path, "%s/%s/%s", root, CRANE_STORAGE_MNT, name);
	container->config_path = malloc(sizeof(name) + sizeof(root) + sizeof(CRANE_CONTAINER_CFG) + 1);
	sprintf(container->config_path, "%s/%s/%s.cfg", root, CRANE_CONTAINER_CFG, name);
	container->veth_host = malloc(sizeof(name) + sizeof(NETNS_VETH_HOST) + 1);
	sprintf(container->veth_host, "%s%s", NETNS_VETH_HOST, name);
	container->veth_guest = malloc(sizeof(name) + sizeof(NETNS_VETH_GUEST) + 1);
	sprintf(container->veth_guest, "%s%s", NETNS_VETH_GUEST, name);
	return container;
}

void
crn_free_container(crn_container *container) {
	if (container != NULL) {
		if (container->diff_path) {
			free(container->diff_path);
		}
		if (container->mnt_path) {
			free(container->mnt_path);
		}
		if (container->config_path) {
			free(container->config_path);
		}
		if (container->veth_host) {
			free(container->veth_host);
		}
		if (container->veth_guest) {
			free(container->veth_guest);
		}
		if (container->network) {
			crn_free_network(container->network);
		}
		free(container);
	}
}

int
crn_init_container(crn_container *container) {
	if (container == NULL) {
		return -1;
	}
	if (crn_mkpath(container->diff_path, DIR_MODE) < 0) {
		crn_err_ret(container->diff_path);
		return -1;
	}
	if (crn_mkpath(container->mnt_path, DIR_MODE) < 0) {
		crn_err_ret(container->mnt_path);
		return -1;
	}
	if (creat(container->config_path, FILE_MODE) < 0) {
		crn_err_ret(container->config_path);
		return -1;
	}
	if (crn_aufs_mount(container->base, container->diff_path, container->mnt_path) < 0) {
		crn_err_ret("aufs mount");
		return -1;
	}
	if (container->network) {
		if (crn_setup_container_veth_pair(container) < 0) {
			crn_err_ret("setup veth pair");
			return -1;
		}
	}
	return 0;
}

int
crn_prepare_mount_in_container() {
	struct crn_mount_entry *entry;

	for (entry = mount_entries; entry->source; entry++) {
		if (mount(entry->source, entry->target, entry->fs, entry->flags, entry->data) < 0) {
			crn_err_sys("mount %s failed.", entry->target);
			return -1;
		}
	}
	return 0;
}

int
crn_prepare_dev_in_container() {
	struct crn_dev_entry *entry;

	dev_t dev;
	for (entry = dev_entries; entry->pathname; entry++) {
		if(access(entry->pathname, F_OK) == -1) {
			dev = makedev(entry->major, entry->minor);
			if (mknod(entry->pathname, entry->mode, dev) < 0) {
				crn_err_sys("mknod %s failed.", entry->pathname);
				return -1;
			}
		}
	}
	return 0;
}

int
crn_set_hostname_in_container(crn_container *container) {
	return sethostname(container->name, sizeof(container->name));
}

int
crn_write_hosts_in_container(crn_container *container) {
	int n, err;
	FILE *fp = NULL;
	struct crn_host_entry *entry;
	char buf[BUFLEN];
	if ((fp = fopen(HOSTS_FILE, "w+")) != NULL) {
		for (entry = host_entries; ; entry++) {
			if (entry->ip) {
				sprintf(buf, "%s\t\t%s\n", entry->ip, entry->hostname);
			}else {
				if (container->network) {
					sprintf(buf, "%s\t\t%s\n", container->network->ip, container->name);
				} else {
					break;
				}
			}
			if (buf && (fputs(buf, fp) == EOF)) {
				err = -1;
				goto out;
			}
			if (!entry->ip) {
				break;
			}
		}
	} else {
		return -1;
	}
out:
	fclose(fp);
	return err;
}

int
crn_endpoint(void *arg) {
	crn_container *container = arg;

	if (sem_wait(sem) < 0) {
		return -1;
	}
	if (sem_unlink(SEM_NAME) < 0) {
		return -1;
	}
	if (sem_close(sem) < 0) {
		return-1;
	}

	if (chdir(container->mnt_path) < 0) {
		crn_err_sys(container->mnt_path);
		return -1;
	}
	if (chroot(container->mnt_path) < 0) {
		crn_err_sys(container->mnt_path);
		return -1;
	}
	if (crn_prepare_mount_in_container() < 0) {
		crn_err_sys("prepare mount failed.");
		return -1;
	}
	if (crn_prepare_dev_in_container() < 0) {
		crn_err_sys("prepare dev failed.");
		return -1;
	}
	if (crn_set_hostname_in_container(container) < 0) {
		crn_err_sys("set hostname %s failed.", container->name);
		return -1;
	}
	if (crn_write_hosts_in_container(container) < 0) {
		crn_err_sys("generate hosts file failed.");
		return -1;
	}
	if (crn_ifup(ETH_LOOPBACK) < 0) {
		crn_err_sys("bring up nic %s failed.", "lo");
		return -1;
	}
	if (container->network) {
		/*sleep(1); //FIXME*/
		if (crn_setup_container_network(container) < 0) {
			crn_err_sys("setup network failed.");
		}
	}
	if (execl("/bin/bash", "/bin/bash", NULL) < 0) {
		return -1;
	}
	return 0;
}

int
crn_run_container(crn_container *container) {
	pid_t pid;
	char *stack, *stack_top;
	char nsfile[BUFLEN], pidpath[BUFLEN];

	stack = malloc(STACK_SIZE);
	stack_top = stack + STACK_SIZE;

    sem = sem_open(SEM_NAME, SEM_OPEN_FLAG, SEM_OPEN_MODE, INIT_V); 
	if (sem == SEM_FAILED) {
		return -1;
	}

	pid = clone(crn_endpoint, stack_top, CLONE_FLAGS, container);

	if (pid == -1) {
		crn_err_ret("clone failed");
		return -1;
	}

	if (sem_post(sem) < 0) {
		return -1;
	}

	container->pid = pid;

	if (crn_setup_container_cgroup(container) < 0) {
		crn_err_ret("setup cgroup failed");
	}

	if (crn_mkpath(NETNS_DIR, DIR_MODE) < 0) {
		crn_err_ret("mkpath failed: %s", NETNS_DIR);
	}
	
	sprintf(nsfile, "%s/%d", NETNS_DIR, pid);
	if(access(nsfile, F_OK) != -1) {
		if (remove(nsfile) < 0) {
			crn_err_ret("remove exist nsfile failed: %s", NETNS_DIR);
			return -1;
		}
	}

	sprintf(pidpath, "/proc/%d/ns/net", pid);
	if (symlink(pidpath, nsfile) < 0) {
		crn_err_ret("link nsfile failed: %s %s", pidpath, nsfile);
		return -1;
	}

	if (container->network) {
		char cmd[BUFLEN];
		sprintf(cmd, "ip link set %s netns %d", container->veth_guest, pid);
		if (system(cmd) < 0) {
			crn_err_ret("set veth netns error");
			return -1;
		}
	}

	waitpid(pid, NULL, 0);
	if (crn_clean_container(container) < 0) {
		crn_err_ret("clean container failed");
		return -1;
	}
	return 0;
}

int
crn_setup_container_veth_pair(crn_container *container) {
	if (crn_create_veth_pair(container->veth_host, container->veth_guest) < 0) {
		goto err;
	}
	if (lxc_bridge_attach(container->network->bridge, container->veth_host) < 0) {
		goto err;
	}
	if (crn_ifup(container->veth_host) < 0) {
		goto err;
	}

	return 0;
err:
	lxc_netdev_delete_by_name(container->veth_host);

	return -1;
}

int
crn_setup_container_network(crn_container *container) {
	int index;
	struct in_addr ip_addr;
	struct in_addr bcast_addr;
	struct in_addr gw_addr;

	if (inet_pton(AF_INET, container->network->ip, &ip_addr) < 0) {
		return -1;
	}
	if (inet_pton(AF_INET, container->network->broadcast, &bcast_addr) < 0) {
		return -1;
	}
	if (inet_pton(AF_INET, container->network->gateway, &gw_addr) < 0) {
		return -1;
	}
	index = if_nametoindex(container->veth_guest);
	if (index < 0) {
		return -1;
	}
	if (lxc_ipv4_addr_add(index, &ip_addr, &bcast_addr, container->network->mask) < 0) {
		return -1;
	}
	if (crn_ifup(container->veth_guest) < 0) {
		return -1;
	}
	if (lxc_ipv4_gateway_add(index, &gw_addr) < 0) {
		return -1;
	}
	return 0;
}

int
crn_write_container_cgroup(crn_container *container, char *mountpoint, char *name, char *value) {
	char buf[BUFLEN];
	sprintf(buf, "%s/crane/%s", mountpoint, container->name);
	return crn_write_cgroup(buf, name, value);
}

int
crn_open_container_cgroup(crn_container *container, char *mountpoint) {
	char buf[BUFLEN], value[BUFLEN];
	sprintf(buf, "%s/crane/%s", mountpoint, container->name);
	if (crn_mkpath(buf, DIR_MODE) < 0) {
		return -1;
	}
	sprintf(value, "%d", container->pid);
	return crn_write_cgroup(buf, "cgroup.procs", value);
}

int
crn_close_container_cgroup(crn_container *container, char *mountpoint) {
	char buf[BUFLEN];
	sprintf(buf, "%s/crane/%s", mountpoint, container->name);
	if(access(buf, F_OK) != -1) {
		return rmdir(buf);	
	}
	return 0;
}

int
crn_setup_container_customize_cgroup(crn_container *container) {
	char *mountpoint = NULL;
	if (container->memory) {
		mountpoint = crn_cgroup_mountpoint("memory");
		if (mountpoint == NULL) {
			return -1;
		}
		if (crn_write_container_cgroup(container, mountpoint, "memory.limit_in_bytes", container->memory) < 0) {
			return -1;
		}
		if (crn_write_container_cgroup(container, mountpoint, "memory.memsw.limit_in_bytes", container->memory) < 0) {
			return -1;
		}
	}
	if (container->cpuset) {
		mountpoint = crn_cgroup_mountpoint("cpuset");
		if ((mountpoint == NULL) ||
				(crn_write_container_cgroup(container, mountpoint, "cpuset.cpus", container->cpuset) < 0)) {
			return -1;
		}
	}
	return 0;
}

int
crn_setup_container_cgroup(crn_container *container) {
	char *subsystem = NULL, *mountpoint = NULL;
	crn_list_t *list = crn_all_subsystems(); 
	CRN_LIST_FOREACH(list, next) {
		subsystem = next->data;
		mountpoint = crn_cgroup_mountpoint(subsystem);
		if (mountpoint != NULL) {
			if (crn_open_container_cgroup(container, mountpoint) < 0) {
				return -1;
			}
		}
	}
	crn_list_destroy(list);
	if (crn_setup_container_customize_cgroup(container) < 0) {
		return -1;
	}
	return 0;
}

int
crn_clean_container_cgroup(crn_container *container) {
	char *subsystem = NULL, *mountpoint = NULL;
	crn_list_t *list = crn_all_subsystems(); 
	CRN_LIST_FOREACH(list, next) {
		subsystem = next->data;
		mountpoint = crn_cgroup_mountpoint(subsystem);
		if (mountpoint != NULL) {
			if (crn_close_container_cgroup(container, mountpoint) < 0) {
				return -1;
			}
		}
	}
	crn_list_destroy(list);
	return 0;
}

int
crn_clean_container(crn_container *container) {
	if (crn_aufs_umount(container->mnt_path) < 0) {
		crn_err_ret(container->mnt_path);
		return -1;
	}
	if (crn_clean_container_cgroup(container) < 0) {
		crn_err_ret("clean cgroup");
		return -1;
	}
	return 0;
}
