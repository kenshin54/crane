#include "crane.h"
#include "crn_container.h"
#include "crn_utils.h"
#include "crn_aufs.h"
#include "crn_network.h"

static struct crn_mount_entry {
	char *source;
	char *target;
	char *fs;
	unsigned long flags;
	void *data;
} mount_entries[] = {
	{ .source = "proc",  .target = "/proc", .fs = "proc",  .flags = 0, .data = NULL },
	{ .source = "sysfs", .target = "/sys",  .fs = "sysfs", .flags = 0, .data = NULL },
	{ .source = NULL,    .target = NULL,    .fs = NULL,    .flags = 0, .data = NULL }
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
	if (aufs_mount(container->base, container->diff_path, container->mnt_path) < 0) {
		crn_err_ret("aufs mount");
		return -1;
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
crn_set_hostname_in_container(crn_container *container) {
	return sethostname(container->name, sizeof(container->name));
}

int
crn_endpoint(void *arg) {
	crn_container *container = arg;
	if (crn_prepare_mount_in_container() < 0) {
		return -1;
	}
	if (crn_set_hostname_in_container(container) < 0) {
		crn_err_sys("set hostname %s failed.", container->name);
		return -1;
	}
	if (crn_ifup("lo") < 0) {
		crn_err_sys("bring up nic %s failed.", "lo");
		return -1;
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
	if (chdir(container->mnt_path) < 0) {
		crn_err_sys(container->mnt_path);
	}
	if (chroot(container->mnt_path) < 0) {
		crn_err_sys(container->mnt_path);
	}
	stack = malloc(STACK_SIZE);
	stack_top = stack + STACK_SIZE;
	pid = clone(crn_endpoint, stack_top, CLONE_FLAGS, container);

	if (pid == -1) {
		crn_err_sys("clone failed");
	}

	waitpid(pid, NULL, 0);
	return 0;
}
