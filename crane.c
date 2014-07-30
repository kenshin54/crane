#include "crane.h"
#include <getopt.h>
#include "crn_container.h"
#include "crn_utils.h"

void
usage(int code) {
	fprintf(stdout,
			"Usage: crane -n name -b base [options]\n\n"
			"Required:\n"
			"\t-n, --name     the name of the container\n"
			"\t-b, --base     the base rootfs path\n"
			"Options:\n"
			"\t-r, --root     the root directory of crane (default: /mnt/crane)\n"
			"\t-h, --help     show this message\n");
	exit(code);
}

void
crn_init(char* base, char* root) {
	char buf[BUFLEN];
	if (crn_dir_accessible(base) < 0) {
		crn_err_sys(base);
	}

	if (crn_dir_accessible(root) < 0) {
		if (crn_mkpath(root, DIR_MODE) < 0) {
			crn_err_sys("create %s failed.", root);
		}
	}

	sprintf(buf, "%s/%s", root, CRANE_STORAGE_DIFF);
	if (crn_mkpath(buf, DIR_MODE) < 0) {
		crn_err_sys("create %s failed.", buf);
	}

	sprintf(buf, "%s/%s", root, CRANE_STORAGE_MNT);
	if (crn_mkpath(buf, DIR_MODE) < 0) {
		crn_err_sys("create %s failed.", buf);
	}

	sprintf(buf, "%s/%s", root, CRANE_CONTAINER_CFG);
	if (crn_mkpath(buf, DIR_MODE) < 0) {
		crn_err_sys("create %s failed.", buf);
	}
}

int
main(int argc, char *argv[]) {
	static struct option longopts[] = {
		{ "name",       required_argument,  NULL,  'n' },
		{ "base",       required_argument,  NULL,  'b' },
		{ "root",       optional_argument,  NULL,  'r' },
		{ NULL,         0,                  NULL,  0 }
	};

	int ch;
	char *name = NULL;
	char *base = NULL;
	char *root = CRANE_ROOT;

	while ((ch = getopt_long(argc, argv, "n:b:r::h", longopts, NULL)) != -1) {
		switch (ch) {
			case 'n':
				name = optarg;
				break;
			case 'b':
				base = optarg;
				break;
			case 'r':
				if (optarg != NULL) {
					root = optarg;
				}
				break;
			case 'h':
				usage(0);
				break;
			case '?':
			default:
				usage(0);
		}
	}
	argc -= optind;
	argv += optind;

	if (name == NULL || base == NULL) {
		usage(1);
	}

	crn_init(base, root);

	crn_container* container = crn_alloc_container(name, base, root);
	if (container == NULL) {
		crn_err_sys("alloc container failed.");
	}

	if (crn_init_container(container) < 0) {
		crn_err_msg("init container failed.");
	}

	if (crn_run_container(container) < 0) {
		crn_err_msg("running container failed.");
	}

	crn_free_container(container);

	return 0;
}

