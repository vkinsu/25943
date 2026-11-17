#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_LINES 1000

struct line {
	off_t offset;
	off_t length;
};

int main(int argc, char *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "using: %s <file>\n", argv[0]);
		return 1;
	}

	int fd = open(argv[1], O_RDONLY);
	if (fd == -1) {
		perror("open");
		return 1;
	}

	struct line table[MAX_LINES];
	int nlines = 0;
	off_t offset = 0;
	off_t pos = 0;
	char c;

	while (read(fd, &c, 1) == 1) {
		if (c == '\n') {
			if (nlines >= MAX_LINES) break;
			table[nlines].offset = offset;
			table[nlines].length = pos - offset + 1;
			nlines++;
			offset = pos + 1;
		}
		pos++;
	}

	if (offset < pos && nlines < MAX_LINES) {
		table[nlines].offset = offset;
		table[nlines].length = pos - offset;
		nlines++;
	}

	printf("lines found: %d\n", nlines);
	for (int i = 0; i < nlines; i++) {
		printf(" line %d: offset=%ld, length=%ld\n", i + 1, (long)table[i].offset, (long)table[i].length);
	}

	int num;
	char buf[4096];
	char line[64];

	while (1) {
	printf("input number of line (0 to exit): ");
	fflush(stdout);
	if (fgets(line, sizeof(line), stdin) == NULL) break;
	if (sscanf(line, "%d", &num) != 1) {
		printf("only 0 to exit\n");
		continue;
	}
	if (num == 0) break;

	if (num < 1 || num > nlines) {
		printf("wrong number\n");
		continue;
	}

	int idx = num - 1;
	off_t off = table[idx].offset;
	off_t len = table[idx].length;

	if (len >= (off_t)sizeof(buf)) {
		printf("line is too long\n");
		continue;
	}

	if (lseek(fd, off, SEEK_SET) == -1) {
		perror("lseek");
		continue;
	}

	ssize_t got = read(fd, buf, len);
	if (got == -1) {
		perror("read");
		continue;
	}

	buf[got] = '\0';
	printf("%s", buf);
	if (got > 0 && buf[got - 1] != '\n') printf("\n");
	}

	close(fd);
	return 0;
}
