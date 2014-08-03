#include "crane.h"
#include <stdarg.h>
#define MAXLINE 4096

static void crn_err_internal(int errnoflag, int error, const char *fmt, va_list ap);

void
crn_err_ret(const char *fmt, ...) {
	va_list		ap;

	va_start(ap, fmt);
	crn_err_internal(1, errno, fmt, ap);
	va_end(ap);
}

void
crn_err_msg(const char *fmt, ...) {
	va_list		ap;

	va_start(ap, fmt);
	crn_err_internal(0, 0, fmt, ap);
	va_end(ap);
}

void
crn_err_quit(const char *fmt, ...) {
	va_list		ap;

	va_start(ap, fmt);
	crn_err_internal(0, 0, fmt, ap);
	va_end(ap);
	exit(1);
}

void
crn_err_sys(const char *fmt, ...) {
	va_list		ap;

	va_start(ap, fmt);
	crn_err_internal(1, errno, fmt, ap);
	va_end(ap);
	exit(1);
}

void
crn_err_exit(int error, const char *fmt, ...) {
	va_list		ap;

	va_start(ap, fmt);
	crn_err_internal(1, error, fmt, ap);
	va_end(ap);
	exit(1);
}

static void
crn_err_internal(int errnoflag, int error, const char *fmt, va_list ap) {
	char	buf[MAXLINE];

	vsnprintf(buf, MAXLINE, fmt, ap);
	if (errnoflag)
		snprintf(buf+strlen(buf), MAXLINE-strlen(buf), ": %s",
		  strerror(error));
	strcat(buf, "\n");
	fflush(stdout);
	fputs(buf, stderr);
	fflush(NULL);
}
