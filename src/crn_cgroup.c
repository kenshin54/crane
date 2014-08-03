#include "crane.h"
#include "crn_cgroup.h"
#include <mntent.h>

#define CGROUPS_INFO_FILE "/proc/cgroups"
#define MAX_CGROUPS_COUNT 20

char *
crn_cgroup_mountpoint(const char* subsystem) {
	struct mntent *ent;
	FILE *file;
	char *path = NULL;

	file = setmntent("/proc/mounts", "r");
	if (file == NULL) {
		crn_err_sys("setmntent");
	}
	while (NULL != (ent = getmntent(file))) {
		if ((strcmp(ent->mnt_type, "cgroup") == 0) && crn_str_endwith(ent->mnt_dir, subsystem)) {
			path = ent->mnt_dir;
			break;
		}
	}
	endmntent(file);
	return path;
}

crn_list_t *
crn_all_subsystems() {
	FILE *fp;
	char *line, *tmp, *node;
	size_t len = 0;
	ssize_t read;
	int i = 0;

	fp = fopen(CGROUPS_INFO_FILE, "r");
	if (fp == NULL) {
		crn_err_ret(CGROUPS_INFO_FILE);
		return NULL;
	}

	crn_list_t *list = crn_list_create();
	while ((read = getline(&line, &len, fp)) != -1) {
		if (line[0] != '#') {
			tmp = strtok(line, "\t \n");
			if (tmp != NULL) {
				node = malloc(sizeof(tmp));
				memcpy(node, tmp, sizeof(tmp));
				crn_list_add(list, node);
			}
		}
	}
	fclose(fp);
	return list;
}

int
crn_write_cgroup(const char *path, const char *name, const char *value) {
	FILE *fp = NULL;
	char fullpath[BUFLEN];
	int ret = 0;
	sprintf(fullpath, "%s/%s", path, name);
	if ((fp = fopen(fullpath, "w+")) == NULL) {
		ret = -1;
	} else {
		ret = fputs(value, fp);
	}
	fclose(fp);
	return ret;
}
