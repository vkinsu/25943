
all: main

.PHONY: all run clean

main: main.c
	gcc main.c -o main -Wall -Wextra -pedantic -std=c99

run: main
	./main sh -c 'exit 7'

clean:
	rm -f main