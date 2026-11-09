CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11 -D_POSIX_C_SOURCE=200809L

all: parallel_min_max process_memory psum zombie

parallel_min_max: parallel_min_max.c utils.c find_min_max.c
	$(CC) $(CFLAGS) -o $@ $^

process_memory: process_memory.c
	$(CC) $(CFLAGS) -no-pie -o $@ $<

libsum.o: libsum.c sum_lib.h
	$(CC) $(CFLAGS) -c -o $@ $<

libsum.a: libsum.o
	ar rcs $@ $^

psum: parallel_sum.c libsum.a sum_lib.h
	$(CC) $(CFLAGS) -pthread -o $@ parallel_sum.c libsum.a

zombie: zombie.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f parallel_min_max process_memory psum zombie libsum.o libsum.a