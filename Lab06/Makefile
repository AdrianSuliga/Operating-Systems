CC = gcc
CFLAGS = -Wall

.PHONY: all clean

all:
	make zad1
	make sender
	make receiver

zad1: zad1.c
	${CC} ${CFLAGS} $^ -o $@

sender: sender.c
	${CC} ${CFLAGS} $^ -o $@

receiver: receiver.c
	${CC} ${CFLAGS} $^ -o $@

clean:
	rm zad1 sender receiver fifo1 fifo2