CC = gcc
CFLAGS = -Wall -Wextra -O2

.PHONY: all clean

all: server32 client32

server32: server32.c
	@LIBS=""; [ "`uname -s`" = "SunOS" ] && LIBS="-lsocket -lnsl"; \
	$(CC) $(CFLAGS) server32.c -o server32 $$LIBS

client32: client32.c
	@LIBS=""; [ "`uname -s`" = "SunOS" ] && LIBS="-lsocket -lnsl"; \
	$(CC) $(CFLAGS) client32.c -o client32 $$LIBS

clean:
	rm -f server client /tmp/uds_32_y.teslenko1_upper.sock
