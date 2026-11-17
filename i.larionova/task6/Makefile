CC = gcc
CFLAGS = -Wall -Wextra -std=gnu11
TARGET = task6
SRC = main.c
FILE ?= file.txt

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET) $(FILE)

clean:
	rm -f $(TARGET)