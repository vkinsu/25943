CC     = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -g
TARGET = ex_time

all: $(TARGET)

$(TARGET): ex_time.c
	$(CC) $(CFLAGS) -o $(TARGET) ex_time.c

clean:
	rm -f $(TARGET) ex_time_solaris *.o core

.PHONY: all clean
