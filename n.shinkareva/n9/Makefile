all: main no_wait

.PHONY: all clean

main: main.c
	gcc main.c -o main -Wall -Wextra -pedantic -std=c99

no_wait: no_wait.c
	gcc no_wait.c -o no_wait -Wall -Wextra -pedantic -std=c99

clean:
	rm -f main no_wait