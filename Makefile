CFLAGS=-ansi -I${CURDIR} -Wall -DMACOS
LDFLAGS=-pthread 

all: obj crane

obj:
	mkdir obj

obj/%.o: src/%.c
	cc -c -o $@ $<

crane: obj/nl.o obj/crn_list.o obj/crn_error.o obj/crn_aufs.o obj/crn_container.o obj/crn_utils.o obj/crn_network.o obj/crn_cgroup.o
	cc $(CFLAGS) $(LDFLAGS) -o $@ src/crane.c $^

clean:
	rm -rf obj crane
