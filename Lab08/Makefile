CC = gcc
CFLAGS = -pthread -lrt

.PHONY: all clean

all:
	make zad8

zad8: zad8.c
	${CC} -o $@ $^ ${CFLAGS}

clean:
	rm zad8