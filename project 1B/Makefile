#
#	'make'			builds the executables client and server from client.c
#					and server.c respectively
#	
#	'make client'	builds only the client executable from client.c
#
#	'make server'	builds only the server executable from server.c
#
#	'make dist'		creates a tarball labeled 'lab1b-704156701.tar.gz
#					that contains only the files 'client.c', 'Makefile', 
#					'README', 'server.c', and 'my.key'
#	
#	'make clean' 	deletes all files ONLY involved in the build process.
#					That is, removes the "client" and "server" executable, 
#					assuming that they existed in the current directory
#

SHELL:=/bin/bash -O extglob
CC = gcc
CFLAGS = -pthread -g -lmcrypt

default: client server

client: client.c
	$(CC) $(CFLAGS) client.c -o client
	
server: server.c
	$(CC) $(CFLAGS) server.c -o server

dist:
	tar -czvf lab1b-704156701.tar.gz client.c server.c my.key README Makefile
clean:
	rm client
	rm server