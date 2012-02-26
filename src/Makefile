CC=gcc
CFLAGS=-W -Wall -g
TFLAG=-lpthread -pthread
CLIENT=client.out
THREAD_SERVER=threadServer.out
SELECT_SERVER=selectServer.out
EPOLL_SERVER=epollServer.out
BUILDDIR=/bin
VPATH=src
SRC=/src

project:

clean:
	rm -f *.o *.bak *.out ex

client: network.o client.o
	$(CC) $(CFLAGS) $(TFLAG) network.o client.o -o $(CLIENT)

threadServer: network.o threadServer.o
	$(CC) $(CFLAGS) $(TFLAG) network.o threadServer.o -o $(THREAD_SERVER)

selectServer: network.o selectServer.o
	$(CC) $(CFLAGS) network.o selectServer.o -o $(SELECT_SERVER)
	
epollServer: network.o epollServer.o
	$(CC) $(CFLAGS) $(TFLAG) network.o epollServer.o -o $(EPOLL_SERVER)

network.o: network.c network.h
	$(CC) $(CFLAGS) -O -c network.c

client.o: client.c
	$(CC) $(CFLAGS) -O -c client.c

threadServer.o: threadServer.c
	$(CC) $(CFLAGS) -O -c threadServer.c

selectServer.o: selectServer.c
	$(CC) $(CFLAGS) -O -c selectServer.c
	
epollServer.o: epollServer.c
	$(CC) $(CFLAGS) -O -c epollServer.c
