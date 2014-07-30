#ifndef CRN_AUFS_H
#define CRN_AUFS_H

#include <sys/mount.h>

int aufs_mount(char *base, char *diff, char *target);
int aufs_umount(char *target);

#endif
