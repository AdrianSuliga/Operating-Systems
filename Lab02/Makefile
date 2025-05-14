CC = gcc
CFLAGS = -Wall -std=c17

.PHONY: all clean

all:
	make client-static

# Static library
bin/collatz.o: lib/collatz.c
	${CC} ${CFLAGS} -c $^ -o $@

# Shared library
bin/libcollatz.so: lib/collatz.c
	${CC} -fPIC -c $^ -o bin/collatz.o
	${CC} -shared -Wl,-soname,libcollatz.so -o $@ bin/collatz.o -lc

client-static: client.c bin/collatz.o
	${CC} ${CFLAGS} $^ -o client

client-shared: client.c bin/libcollatz.so
	${CC} $< -L./bin -lcollatz -o client -Wl,-rpath=./bin

client-dynamic: client.c bin/libcollatz.so
	${CC} -D DYNAMIC $< -L./bin -lcollatz -o client -Wl,-rpath=./bin

clean: 
	rm client bin/*.o bin/*.so