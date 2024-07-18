CC=gcc
CFLAGS=-Wall

all: build

build: quadtree

quadtree: quadtree.o
	$(CC) -o quadtree quadtree.o

quadtree.o: quadtree.c
	$(CC) -c -o quadtree.o quadtree.c $(CFLAGS)

.PHONY: clean
clean:
	rm -rf *.o quadtree
        
