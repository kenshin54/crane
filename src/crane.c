#include "crane.h"
#include <getopt.h>
#include "crn_utils.h"
#include "crn_network.h"
#include "crn_cgroup.h"
#include "crn_container.h"

void
usage(int code) {
	fprintf(stdout,
			"Usage: crane -n name -b base [options]\n\n"
			"Required:\n"
			"\t-n, --name		Set name of the container\n"
			"\t-i, --image		Base image path\n"
			"Options:\n"
			"\t-r, --root		Set root directory of crane (default: /mnt/crane)\n"
			"\t-b, --bridge		Attach containers to a pre-existing network bridge\n"
			"\t-t, --network	Setup network (format: format: <ipaddr>/<subnet>@<default_gateway>#<broadcast>)\n"
			"\t-m, --memory		Memory limit (format: <number><optional unit>, where unit = b, k, m or g)\n"
			"\t-c, --cpuset		CPUs in which to allow execution (0-3, 0,1)\n"
			"\t-h, --help		Show this message\n");
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
		{ "name",		required_argument,	NULL,	'n' },
		{ "image",		required_argument,	NULL,	'i' },
		{ "root",		optional_argument,	NULL,	'r' },
		{ "bridge",		optional_argument,	NULL,	'b'	},
		{ "network",	optional_argument,	NULL,	't'	},
		{ "memory",		optional_argument,	NULL,	'm'	},
		{ "cpuset",		optional_argument,	NULL,	'c'	},
		{ NULL,			0,					NULL,	0	}
	};

	int ch;
	char *name = NULL;
	char *image = NULL;
	char *root = CRANE_ROOT;
	char *bridge = NULL;
	char *network = NULL;
	char *memory = NULL;
	char *cpuset = NULL;

	while ((ch = getopt_long(argc, argv, "n:i:b:r::t::m::c::h", longopts, NULL)) != -1) {
		switch (ch) {
			case 'n':
				name = optarg;
				break;
			case 'i':
				image = optarg;
				break;
			case 'r':
				if (optarg != NULL) {
					root = optarg;
				}
				break;
			case 'b':
				if (optarg != NULL) {
					bridge = optarg;
				}
				break;
			case 't':
				if (optarg != NULL) {
					if (bridge == NULL) {
						crn_err_quit("bridge is required if you want setup network info.");
					}
					network = optarg;
				}
				break;
			case 'm':
				if (optarg != NULL) {
					memory = optarg;
				}
				break;
			case 'c':
				if (optarg != NULL) {
					cpuset = optarg;
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

	if (name == NULL || image == NULL) {
		usage(1);
	}

	crn_init(image, root);

	crn_container* container = crn_alloc_container(name, image, root);
	if (container == NULL) {
		crn_err_sys("alloc container failed.");
	}

	if (network && bridge) {
		crn_network_t *net = crn_create_network(network, bridge);
		if (net == NULL) {
			crn_err_quit("network create error.");
		}
		container->network = net;
	}

	container->memory = memory;
	container->cpuset = cpuset;

	if (crn_init_container(container) < 0) {
		crn_err_sys("init container failed.");
	}

	if (crn_run_container(container) < 0) {
		crn_err_sys("running container failed.");
	}

	crn_free_container(container);

	return 0;
}

