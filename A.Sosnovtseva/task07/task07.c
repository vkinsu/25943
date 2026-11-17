#define _POSIX_C_SOURCE 200809L

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

#define MAX_LINES 1000

typedef struct {
    size_t offset;
    size_t length;
} line_t;

static volatile sig_atomic_t time_is_over = 0;

static void timeout(int sig)
{
    (void)sig;
    time_is_over = 1;
}

int main(int argc, char **argv)
{
    line_t table[MAX_LINES];
    struct stat info;
    struct sigaction action = {0};
    char *data = NULL, input[100], *end;
    size_t start = 0, size;
    int lines = 0, first_input = 1, status = EXIT_FAILURE;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        return EXIT_FAILURE;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    if (fstat(fd, &info) == -1) {
        perror("fstat");
        close(fd);
        return EXIT_FAILURE;
    }

    size = (size_t)info.st_size;
    if (size > 0) {
        data = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (data == MAP_FAILED) {
            perror("mmap");
            close(fd);
            return EXIT_FAILURE;
        }
    }
    close(fd);

    for (size_t i = 0; i < size; ++i) {
        if (lines == MAX_LINES) {
            fprintf(stderr, "Too many lines\n");
            goto cleanup;
        }
        if (data[i] == '\n') {
            table[lines++] = (line_t){start, i - start + 1};
            start = i + 1;
        }
    }

    if (start < size)
        table[lines++] = (line_t){start, size - start};

    printf("Номер\tСмещение\tДлина (байт)\n");
    for (int i = 0; i < lines; ++i) {
        printf("%d\t%zu\t\t%zu\n",
               i + 1, table[i].offset, table[i].length);
    }

    action.sa_handler = timeout;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGALRM, &action, NULL) == -1) {
        perror("sigaction");
        goto cleanup;
    }

    printf("Line number (5 seconds, 0 to quit): ");
    fflush(stdout);
    alarm(5);

    for (;;) {
        char *answer = NULL;
        if (!time_is_over)
            answer = fgets(input, sizeof(input), stdin);

        if (time_is_over) {
            puts("\nTime is over. Complete file:");
            if (size > 0) {
                fwrite(data, 1, size, stdout);
                if (data[size - 1] != '\n')
                    putchar('\n');
            }
            break;
        }
        if (answer == NULL)
            break;

        errno = 0;
        long number = strtol(input, &end, 10);
        int invalid = (end == input || errno == ERANGE);
        while (isspace((unsigned char)*end))
            ++end;

        if (invalid || *end != '\0' || number < 0 || number > lines) {
            printf("Enter a number from 0 to %d\n", lines);
        } else {
            if (first_input) {
                alarm(0);
                first_input = 0;
            }
            if (number == 0)
                break;

            line_t line = table[number - 1];
            fwrite(data + line.offset, 1, line.length, stdout);
            if (data[line.offset + line.length - 1] != '\n')
                putchar('\n');
        }

        printf("Line number (0 to quit): ");
        fflush(stdout);
    }

    status = EXIT_SUCCESS;

cleanup:
    alarm(0);
    if (size > 0 && munmap(data, size) == -1) {
        perror("munmap");
        status = EXIT_FAILURE;
    }
    return status;
}