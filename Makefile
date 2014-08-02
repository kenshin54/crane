CFLAGS=-ansi -I${CURDIR} -Wall -DMACOS

all: crane

%.o: %.c
	cc -c -o $@ $<

crane: nl.o crn_list.o crn_error.o crn_aufs.o crn_container.o crn_utils.o crn_network.o crn_cgroup.o

clean:
	rm -rf nl.o crn_list.o crn_error.o crn_aufs.o crn_container.o crn_utils.o crn_network.o crn_cgroup.o crane
