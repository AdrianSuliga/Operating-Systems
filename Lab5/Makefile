CC = gcc
CFLAGS = -Wall

all:
	make zad1
	make sender
	make catcher

zad1: zad1.c
	${CC} ${CFLAGS} $^ -o $@

sender: sender.c
	${CC} ${CFLAGS} $^ -o $@

catcher: catcher.c
	${CC} ${CFLAGS} $^ -o $@

clean:
	rm sender catcher