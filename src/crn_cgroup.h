#ifndef CRN_CGROUP_H
#define CRN_CGROUP_H
#include "crn_list.h"

char *crn_cgroup_mountpoint(const char *subsystem);
crn_list_t *crn_all_subsystems();
int crn_write_cgroup(const char *path, const char *name, const char *value);

#endif
