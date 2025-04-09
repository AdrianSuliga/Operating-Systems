CC = gcc
CFLAGS = -Wall

.PHONY: all clean run

all: zad1

bibl1.so: bibl1.c
	${CC} ${CFLAGS} -fPIC -c $^ -o bibl1.o
	${CC} -shared -o $@ bibl1.o -lc

zad1: zad1.c bibl1.so
	${CC} ${CFLAGS} $< ./bibl1.so -o $@

clean:
	rm -f zad1 *.o *.so

run: zad1
	LD_LIBRARY_PATH=./ ./zad1