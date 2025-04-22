CC = gcc
CFLAGS = -Wall

.PHONY: all clean test

all: 
	make bin/zad1

lib/bibl1.so: bibl1.c
	${CC} ${CFLAGS} -fPIC -c $^ -o lib/bibl1.o
	${CC} -shared -o $@ lib/bibl1.o -lc

bin/zad1: zad1.c lib/bibl1.so
	${CC} ${CFLAGS} $< ./lib/bibl1.so -o $@ -ldl

clean:
	rm -f zad1 lib/* bin/*

test: bin/zad1
	LD_LIBRARY_PATH=/ ./bin/zad1
