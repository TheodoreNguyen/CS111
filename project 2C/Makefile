
SHELL:=/bin/bash -O extglob
CC = gcc
CFLAGS = -pthread -g 
TARGET = lab2c
default: lab2c

lab2c: 
	$(CC) $(CFLAGS) SortedList.c lab2c.c -o lab2c

gprof:
	$(CC) $(CFLAGS) -pg SortedList.c lab2c.c -o lab2c_gprof
	
clean:
	rm lab2c

dist:
	tar -czvf lab2c-704156701.tar.gz lab2c.c SortedList.c SortedList.h Makefile README graph.PNG

check:
	./lab2c --threads=1 --iterations=1000 --lists=10 --yield=ids
	./lab2c --threads=10 --iterations=1000 --lists=10 --yield=ids --sync=m
	./lab2c --threads=10 --iterations=1000 --lists=10 --yield=ids --sync=s