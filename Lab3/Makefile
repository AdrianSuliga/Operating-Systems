CC = gcc
CFLAGS = -Wall 

.PHONY: all clean

all:
	make reverse

reverse: reverse.c
	${CC} ${CFLAGS} $^ -o $@

clean:
	rm reverse file2.txt file3.txt