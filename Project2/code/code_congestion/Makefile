CXXFLAGS= -Wall

all: server client

server: server.c
	gcc $(CXXFLAGS) -I. server.c -o server

client: client.c
	gcc $(CXXFLAGS) -I. client.c -o client

clean:
	rm client server testOut.png