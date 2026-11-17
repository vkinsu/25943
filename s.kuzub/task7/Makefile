CC     = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -g
TARGET = main

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $(TARGET) main.c

clean:
	rm -f $(TARGET) *.o core

.PHONY: all clean
