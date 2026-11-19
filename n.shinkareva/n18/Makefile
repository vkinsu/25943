all: main

.PHONY: all run clean

main: main.c
	gcc main.c -o main -Wall -Wextra -pedantic -std=c99

run: main
	./main main.c Makefile .

clean:
	rm -f main