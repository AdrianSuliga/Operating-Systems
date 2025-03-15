CC = gcc
CFLAGS = -Wall -std=c17

.PHONY: all clean

all: 
	make countdown

countdown: countdown.c
	${CC} ${CFLAGS} $^ -o $@

clean:
	rm -f countdown