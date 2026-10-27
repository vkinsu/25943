CC = gcc
CFLAGS = -Wall -Wextra -std=c11
TARGET = lines_list
SRC = list.c

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)