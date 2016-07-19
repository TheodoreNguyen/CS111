
SHELL:=/bin/bash -O extglob
CC = gcc
CFLAGS = -g 
TARGET = lab3a
default: lab3a

lab3c:
	$(CC) $(CFLAGS) lab3a.c -o lab3a

dist:
	tar -czvf lab3a-704156701.tar.gz lab3a.c README Makefile
	
clean:
	rm lab3a

superclean:
	rm !(.|..|lab3a.c|Makefile|README)
