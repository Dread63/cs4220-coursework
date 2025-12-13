CC = gcc
FLAGS = -lssl -lcrypto # Requried for OpenSSL

all: server client

server: server.c
	$(CC) -o server server.c $(FLAGS)

client: client.c
	$(CC) -o client client.c $(FLAGS)

clean:
	rm -f server client
