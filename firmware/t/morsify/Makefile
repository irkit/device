
all: build test

build:
	gcc -Wall -c -g -o ./CRC8.o CRC8.c
	g++ -Wall -g -o ./test.out test.cpp CRC8.o

test:
	./test.out
