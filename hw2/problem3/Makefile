all: bandwidth_test_small bandwidth_test_large test

CC=gcc
CFLAGS=-O3

bandwidth_test_small: bandwidth_test.c
	$(CC) $(CFLAGS) -o  bandwidth_test_small bandwidth_test.c

bandwidth_test_large: bandwidth_test_large.c
	$(CC) $(CFLAGS) -o bandwidth_test_large bandwidth_test_large.c

test: test.c
	$(CC) $(CFLAGS) -o test test.c