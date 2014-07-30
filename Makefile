CFLAGS=-ansi -I${CURDIR} -Wall -DMACOS

all: crane

%.o: %.c
	cc -c -o $@ $<

crane: crn_error.o crn_aufs.o crn_container.o crn_utils.o crn_network.o

clean:
	rm -rf crn_error.o crn_aufs.o crn_container.o crn_utils.o crn_network.o crane
