#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#define MAXLINES 1000

int main(int argc, char *argv[])
{
    off_t offs[MAXLINES];   /* line start offsets */
    int lens[MAXLINES];     /* line lengths without '\n' */
    int n = 0, i, num;
    off_t start = 0, pos;
    char c, *buf;
    int fd;

    if (argc < 2) {
        fprintf(stderr, "usage: %s file\n", argv[0]);
        return 1;
    }
    fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror(argv[1]);
        return 1;
    }

    /* build table */
    while (n < MAXLINES && read(fd, &c, 1) == 1) {
        if (c == '\n') {
            pos = lseek(fd, 0L, SEEK_CUR);  /* position after '\n' */
            offs[n] = start;
            lens[n] = pos - 1 - start;
            n++;
            start = pos;
        }
    }
    /* last line without '\n' */
    pos = lseek(fd, 0L, SEEK_CUR);
    if (n < MAXLINES && pos > start) {
        offs[n] = start;
        lens[n] = pos - start;
        n++;
    }

    /* debug: print table */
    printf("line\toffset\tlength\n");
    for (i = 0; i < n; i++)
        printf("%d\t%ld\t%d\n", i + 1, (long)offs[i], lens[i]);

    for (;;) {
        printf("Line number: ");
        fflush(stdout);
        if (scanf("%d", &num) != 1) {
            if (feof(stdin))
                break;
            scanf("%*s");   /* skip bad input */
            printf("bad input\n");
            continue;
        }
        if (num == 0)
            break;
        if (num < 0 || num > n) {
            printf("no such line (1..%d)\n", n);
            continue;
        }

        buf = malloc(lens[num - 1] + 1);
        if (buf == NULL) {
            perror("malloc");
            break;
        }
        lseek(fd, offs[num - 1], SEEK_SET);
        if (read(fd, buf, lens[num - 1]) != lens[num - 1]) {
            perror("read");
            free(buf);
            continue;
        }
        buf[lens[num - 1]] = '\0';
        printf("%s\n", buf);
        free(buf);
    }

    close(fd);
    return 0;
}
