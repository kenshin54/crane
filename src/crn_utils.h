#ifndef CRN_UTILS_H
#define CRN_UTILS_H

int crn_dir_accessible(char *path);
int crn_mkpath(char *s, mode_t mode);
int crn_str_endwith(char* base, char* str);
char *crn_strncpy(char *src, size_t n);

#endif
