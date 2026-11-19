all: main

.PHONY: all clean run

main: main.c
	gcc main.c -o main -Wall -Wextra -pedantic -std=c99

run: main
	./main

clean:
	rm -f main