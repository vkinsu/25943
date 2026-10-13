
CC = gcc
CFLAGS = -Wall -Wextra

PROGRAMS = section1_task1 section1_task2 section1_task3 section1_task4 section1_task5 section1_task6 section1_task7

all: $(PROGRAMS)

section1_task1: section1_task1.c
	$(CC) $(CFLAGS) -o section1_task1 section1_task1.c

section1_task2: section1_task2.c
	$(CC) $(CFLAGS) -o section1_task2 section1_task2.c

section1_task3: section1_task3.c
	$(CC) $(CFLAGS) -o section1_task3 section1_task3.c

section1_task4: section1_task4.c
	$(CC) $(CFLAGS) -o section1_task4 section1_task4.c

section1_task5: section1_task5.c input.h
	$(CC) $(CFLAGS) -o section1_task5 section1_task5.c

section1_task6: section1_task6.c input.h
	$(CC) $(CFLAGS) -o section1_task6 section1_task6.c

section1_task7: section1_task7.c input.h
	$(CC) $(CFLAGS) -o section1_task7 section1_task7.c

clean:
	rm -f $(PROGRAMS)