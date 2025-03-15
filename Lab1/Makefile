CC = gcc
CFLAGS = -Wall -std=c17 -g

.PHONY: all clean

all: 
	make countdown

countdown: countdown.c
	${CC} ${CFLAGS} $^ -o $@

clean:
	rm -f countdown