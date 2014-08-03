#ifndef CRANE_H
#define CRANE_H

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sched.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mount.h>

#define CLONE_FLAGS CLONE_NEWUTS|CLONE_NEWIPC|CLONE_NEWNET|CLONE_NEWNS|CLONE_NEWPID|SIGCHLD
#define	FILE_MODE	(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define	DIR_MODE	(FILE_MODE | S_IXUSR | S_IXGRP | S_IXOTH)
#define	BUFLEN 1024

#define STACK_SIZE (1024 * 1024)
#define CRANE_ROOT "/mnt/crane"
#define CRANE_STORAGE_DIFF "aufs/diff"
#define CRANE_STORAGE_MNT "aufs/mnt"
#define CRANE_CONTAINER_CFG "containers"


void crn_err_ret(const char *fmt, ...);
void crn_err_msg(const char *fmt, ...);
void crn_err_quit(const char *fmt, ...);
void crn_err_sys(const char *fmt, ...);
void crn_err_exit(int error, const char *fmt, ...);

#endif
