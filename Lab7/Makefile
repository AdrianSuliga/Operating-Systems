CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -lrt -pthread

all: 
	make server
	make client

server: server.c 
	$(CC) $(CFLAGS) -o server server.c $(LDFLAGS)

client: client.c 
	$(CC) $(CFLAGS) -o client client.c $(LDFLAGS)

clean:
	rm -f server client