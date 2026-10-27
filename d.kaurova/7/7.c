#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>

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

    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("fstat");
        close(fd);
        return 1;
    }

    size_t size = st.st_size;
    if (size == 0) {
        printf("lines found: 0\n");
        close(fd);
        return 0;
    }

    char *data = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    struct line table[MAX_LINES];
    int nlines = 0;
    off_t offset = 0;

    for (size_t i = 0; i < size; i++) {
        if (data[i] == '\n') {
            if (nlines >= MAX_LINES) break;
            table[nlines].offset = offset;
            table[nlines].length = i - offset + 1;
            nlines++;
            offset = i + 1;
        }
    }

    if (offset < (off_t)size && nlines < MAX_LINES) {
        table[nlines].offset = offset;
        table[nlines].length = size - offset;
        nlines++;
    }

    printf("lines found: %d\n", nlines);
    for (int i = 0; i < nlines; i++) {
        printf(" line %d: offset=%ld, length=%ld\n",
               i + 1, (long)table[i].offset, (long)table[i].length);
    }

    int num;
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

        fwrite(data + off, 1, len, stdout);
        if (len > 0 && data[off + len - 1] != '\n') printf("\n");
    }

    munmap(data, size);
    close(fd);
    return 0;
}
