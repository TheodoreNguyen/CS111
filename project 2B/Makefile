#
#	'make'
#



SHELL:=/bin/bash -O extglob
CC = gcc
CFLAGS = -pthread -g 
TARGET = lab2b
default: lab2b

lab2b: 
	$(CC) $(CFLAGS) SortedList.c lab2b.c -o lab2b

clean:
	rm lab2b

dist:
	tar -czvf lab2b-704156701.tar.gz lab2b.c SortedList.c SortedList.h Makefile README graph1.PNG graph2.PNG

SortedListTest: 
	gcc -g -pthread SortedList.c SortedListMain.c -o SortedTest
	./SortedTest
	rm SortedTest
