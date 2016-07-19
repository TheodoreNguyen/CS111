
TARGET = lab3b
default: lab3b

lab3b:
	cp lab3b.py lab3b
	chmod u+x lab3b

run:
	./lab3b
	
check:
	./lab3b

dist:
	tar -czvf lab3b-704156701.tar.gz lab3b.py README Makefile
	
clean: 
	rm lab3b

test: 
	cp lab3b.py lab3b
	chmod u+x lab3b
	./lab3b
	python -i lab3b.py