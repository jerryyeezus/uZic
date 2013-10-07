#################################################################
##
## FILE:	Makefile
## PROJECT:	CS 3251 Project 1 - Professor Traynor
## DESCRIPTION: Compile Project 1
##
#################################################################

CC=gcc

OS := $(shell uname -s)

# Extra LDFLAGS if Solaris
ifeq ($(OS), SunOS)
	LDFLAGS=-lsocket -lnsl
endif

all: client server 

client: client.c
	$(CC) musicEncoding.c client.c -o uZic -g -lpthread

server: server.c
	$(CC) musicEncoding.c server.c -o uZicServer -g -lpthread

clean:
	rm -f client server *.o

