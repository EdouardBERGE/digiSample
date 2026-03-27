CC=cc
CFLAGS=-O2 -lm
convert: convert.c
	$(CC) convert.c $(CFLAGS) -o convert

