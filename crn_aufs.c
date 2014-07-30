#include "crane.h"
#include "crn_aufs.h"
#include <mntent.h>

int
aufs_mount(char *base, char *diff, char *target) {
	if (!is_mounted(target)) {
		char buf[BUFLEN];
		sprintf(buf, "br:%s=rw:%s=ro+wh:,xino=/dev/shm/aufs.xino", diff, base);
		return mount("none", target, "aufs", MS_RELATIME, buf);
	}
	return 0;
}

int
aufs_umount(char *target) {
	return umount(target);
}

int is_mounted(char *target) {
	struct mntent *ent;
	FILE *file;

	file = setmntent("/proc/mounts", "r");
	if (file == NULL) {
		crn_err_sys("setmntent");
	}
	while (NULL != (ent = getmntent(file))) {
		if (strcmp(ent->mnt_dir, target) == 0) {
			return 1;
		}
	}
	endmntent(file);
	return 0;
}
