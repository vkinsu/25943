#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#define MAXLINES 1000

static volatile sig_atomic_t timeout = 0;

static void on_alarm(int sig)
{
    timeout = 1;
}

int main(int argc, char *argv[])
{
    long offs[MAXLINES]; 
    int lens[MAXLINES];
    int n = 0, i, num, r, first = 1;
    long start = 0, size, k;
    char *map;
    int fd;
    struct stat st;
    struct sigaction sa;

    if (argc < 2) {
        fprintf(stderr, "usage: %s file\n", argv[0]);
        return 1;
    }
    fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror(argv[1]);
        return 1;
    }
    if (fstat(fd, &st) == -1) {
        perror("fstat");
        return 1;
    }
    size = st.st_size;
    if (size == 0) {
        printf("empty file\n");
        return 0;
    }

    map = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
        perror("mmap");
        return 1;
    }
    close(fd); 

   
    for (k = 0; k < size && n < MAXLINES; k++) {
        if (map[k] == '\n') {
            offs[n] = start;
            lens[n] = k - start;
            n++;
            start = k + 1;
        }
    }
    
    if (n < MAXLINES && start < size) {
        offs[n] = start;
        lens[n] = size - start;
        n++;
    }

    //print table
    printf("line\toffset\tlength\n");
    for (i = 0; i < n; i++)
        printf("%d\t%ld\t%d\n", i + 1, offs[i], lens[i]);

   
    sa.sa_handler = on_alarm;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGALRM, &sa, NULL);

    for (;;) {
        printf("Line number (5 sec): ");
        fflush(stdout);

        
        alarm(5);
        r = scanf("%d", &num);
        alarm(0);

        if (timeout) {
            printf("\n");
            fwrite(map, 1, size, stdout);
            break;
        }
        if (r != 1) {
            if (feof(stdin))
                break;
            scanf("%*s"); 
            printf("bad input\n");
            continue;
        }
        first = 0;

        if (num == 0)
            break;
        if (num < 0 || num > n) {
            printf("no such line (1..%d)\n", n);
            continue;
        }
        printf("%.*s\n", lens[num - 1], map + offs[num - 1]);
    }

    munmap(map, size);
    return 0;
}
