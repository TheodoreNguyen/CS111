#
#	'make'			builds the executable file 'lab1a' 
#
#	'make dist'		creates a tarball labeled 'lab1a-704156701.tar.gz
#					that contains only the files 'lab1a.c', 'Makefile', 
#					and 'README'
#	
#	'make clean' 	deletes all files in the current directory except for
#					'lab1a.c', 'Makefile', and 'README'
#
SHELL:=/bin/bash -O extglob
CC = gcc
CFLAGS = -pthread -g
TARGET = lab1a
default: lab1a

lab1a: 
	$(CC) $(CFLAGS) lab1a.c -o lab1a
dist:
	tar -czvf lab1a-704156701.tar.gz lab1a.c Makefile README
clean:
	rm !(.|..|lab1a.c|Makefile|README)