CC = gcc
CFLAGS = -Wall

.PHONY: all clean

all:
	make shm_read
	make shm_write

shm_read: shm_read.c
	${CC} ${CFLAGS} $^ -o $@

shm_write: shm_write.c
	${CC} ${CFLAGS} $^ -o $@

clean:
	rm shm_read shm_write