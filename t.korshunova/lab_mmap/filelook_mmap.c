#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define MAXLINES 100

int main(int argc, char *argv[])
{
    int fd;
    struct stat st;
    char *data;
    long offsets[MAXLINES];
    int lengths[MAXLINES];
    int nlines = 0;
    int i, num;

    if (argc < 2) {
        printf("Usage: %s file\n", argv[0]);
        return 1;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    if (fstat(fd, &st) < 0) {
        perror("fstat");
        close(fd);
        return 1;
    }

    data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    offsets[0] = 0;
    for (i = 0; i < st.st_size; i++) {
        if (nlines >= MAXLINES)
            break;
        if (data[i] == '\n') {
            lengths[nlines] = i - offsets[nlines];
            nlines++;
            offsets[nlines] = i + 1;
        }
    }
    if (nlines < MAXLINES && st.st_size > offsets[nlines]) {
        lengths[nlines] = st.st_size - offsets[nlines];
        nlines++;
    }

    printf("Built table: %d lines\n", nlines);
    for (i = 0; i < nlines; i++) {
        printf("Line %d: offset=%ld length=%d\n", i + 1, offsets[i], lengths[i]);
    }

    while (1) {
        printf("\nEnter line number (0 to quit): ");
        fflush(stdout);
        if (scanf("%d", &num) != 1)
            break;
        if (num == 0)
            break;
        if (num < 1 || num > nlines) {
            printf("No such line\n");
            continue;
        }

        fwrite(data + offsets[num - 1], 1, lengths[num - 1], stdout);
        printf("\n");
    }

    munmap(data, st.st_size);
    close(fd);
    return 0;
}
