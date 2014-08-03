#ifndef CRN_AUFS_H
#define CRN_AUFS_H

#include <sys/mount.h>

int crn_aufs_mount(char *base, char *diff, char *target);
int crn_aufs_umount(char *target);

#endif
