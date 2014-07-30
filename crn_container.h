#ifndef CRN_CONTAINER_H
#define CRN_CONTAINER_H

typedef struct {
	char *name;
	char *base;
	char *root;
	char *diff_path;
	char *mnt_path;
	char *config_path;
} crn_container;

crn_container* crn_alloc_container(char *name, char *base, char *root);
void crn_free_container(crn_container *container);
int crn_init_container(crn_container *container);
int crn_run_container(crn_container *container);

#endif
