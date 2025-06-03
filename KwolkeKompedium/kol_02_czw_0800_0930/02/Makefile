CC = gcc
CFLAGS = -Wall

.PHONY: all clean

all:
	make pipe

pipe: pipe.c
	${CC} ${CFLAGS} $^ -o $@

clean:
	rm pipe