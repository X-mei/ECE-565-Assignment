all: bandwidth_test_small bandwidth_test_large

CC=gcc
CFLAGS=-O3

bandwidth_test_small: bandwidth_test.c
	$(CC) -o bandwidth_test_small bandwidth_test.c

bandwidth_test_large: bandwidth_test_large.c
	$(CC) -o bandwidth_test_large bandwidth_test_large.c