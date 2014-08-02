#include "crane.h"
#include "crn_utils.h"
#include <dirent.h>
#include <libgen.h>

int
crn_dir_accessible(char *path) {
	DIR* dir = opendir(path);
	if (dir) {
		closedir(dir);
		return 0;
	} else {
		return -1;
	}
}

int
crn_mkpath(char *s, mode_t mode){
	char *q, *r = NULL, *path = NULL, *up = NULL;
	int rv;

	rv = -1;
	if (strcmp(s, ".") == 0 || strcmp(s, "/") == 0)
		return (0);

	if ((path = strdup(s)) == NULL)
		exit(1);

	if ((q = strdup(s)) == NULL)
		exit(1);

	if ((r = dirname(q)) == NULL)
		goto out;

	if ((up = strdup(r)) == NULL)
		exit(1);

	if ((crn_mkpath(up, mode) == -1) && (errno != EEXIST))
		goto out;

	if ((mkdir(path, mode) == -1) && (errno != EEXIST))
		rv = -1;
	else
		rv = 0;

out:
	if (up != NULL)
		free(up);
	free(q);
	free(path);
	return (rv);
}

int
crn_str_endwith(char* base, char* str) {
    int blen = strlen(base);
    int slen = strlen(str);
    return (blen >= slen) && (0 == strcmp(base + blen - slen, str));
}

char *
crn_strncpy(char *src, size_t n) {
	char *dest = malloc(n + 1);
	strncpy(dest, src, n);
	dest[n] = '\0';
	return dest;
}
