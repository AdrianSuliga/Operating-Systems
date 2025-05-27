CC = gcc
CFLAGS = -Wall

.PHONY: all clean

all:
	make main

main: main.c
	${CC} $^ -o $@ -pthread

clean:
	rm main