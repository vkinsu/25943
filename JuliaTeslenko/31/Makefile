CC = gcc
CFLAGS = -Wall -Wextra -O2

.PHONY: all clean

all: server client

server: server31.c
	@LIBS=""; [ "`uname -s`" = "SunOS" ] && LIBS="-lsocket -lnsl"; \
	$(CC) $(CFLAGS) server31.c -o server31 $$LIBS

client: client31.c
	@LIBS=""; [ "`uname -s`" = "SunOS" ] && LIBS="-lsocket -lnsl"; \
	$(CC) $(CFLAGS) client31.c -o client31 $$LIBS

clean:
	rm -f server client /tmp/uds_31_y.teslenko1_upper.sock
