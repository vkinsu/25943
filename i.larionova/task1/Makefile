CC = gcc
CFLAGS = -Wall -Wextra -std=gnu11 -D_GNU_SOURCE
TARGET = main
SRC = main.c

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET) $(ARGS)

clean:
	rm -f $(TARGET)