CC = gcc
CFLAGS = -Wall -Wextra

PROGRAMS = section1_task1 section1_task2 section1_task3 section1_task4 section1_task5 section1_task6 section1_task7

all: $(PROGRAMS)

section1_task%: section1_task%.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(PROGRAMS)

.PHONY: all clean
