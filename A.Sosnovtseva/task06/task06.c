#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <inttypes.h>

#define MAX_LINES 1000
#define MAX_LINE_LENGTH 4096

typedef struct {
    off_t offset;
    size_t length;
} line_t;

static int fd;

static void timeout(int sig)
{
    const char message[] = "\nTime is over.\n";

    (void)sig;
    write(STDOUT_FILENO, message, sizeof(message) - 1);
    close(fd);
    _exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
    line_t table[MAX_LINES];
    char c, text[MAX_LINE_LENGTH], input[100], *end;
    off_t start = 0, position;
    ssize_t result;
    int lines = 0, first_input = 1, status = EXIT_FAILURE;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s file\n", argv[0]);
        return EXIT_FAILURE;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    if (signal(SIGALRM, timeout) == SIG_ERR) {
        perror("signal");
        goto cleanup;
    }

    while ((result = read(fd, &c, 1)) == 1) {
        if (lines == MAX_LINES) {
            fprintf(stderr, "Too many lines\n");
            goto cleanup;
        }
        if (c != '\n')
            continue;

        position = lseek(fd, 0L, SEEK_CUR);
        if (position == (off_t)-1) {
            perror("lseek");
            goto cleanup;
        }

        table[lines++] = (line_t){start, (size_t)(position - start)};
        start = position;
    }

    if (result == -1) {
        perror("read");
        goto cleanup;
    }

    position = lseek(fd, 0L, SEEK_CUR);
    if (position == (off_t)-1) {
        perror("lseek");
        goto cleanup;
    }

    if (position > start)
        table[lines++] = (line_t){start, (size_t)(position - start)};

    printf("Номер\tСмещение\tДлина (байт)\n");
    for (int i = 0; i < lines; ++i) {
        printf("%d\t%jd\t\t%zu\n",
               i + 1, (intmax_t)table[i].offset, table[i].length);
    }

    printf("Line number (5 seconds, 0 to quit): ");
    fflush(stdout);
    alarm(5);

    for (;;) {
        if (fgets(input, sizeof(input), stdin) == NULL)
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

            if (line.length >= sizeof(text)) {
                puts("Line is too long");
            } else {
                if (lseek(fd, line.offset, SEEK_SET) == (off_t)-1) {
                    perror("lseek");
                    goto cleanup;
                }

                size_t total = 0;
                while (total < line.length) {
                    result = read(fd, text + total, line.length - total);

                    if (result < 0 && errno == EINTR)
                        continue;

                    if (result <= 0) {
                        fprintf(stderr, "Cannot read complete line\n");
                        goto cleanup;
                    }

                    total += (size_t)result;
                }

                text[total] = '\0';
                printf("%s", text);

                if (total == 0 || text[total - 1] != '\n')
                    putchar('\n');
            }
        }

        printf("Line number (0 to quit): ");
        fflush(stdout);
    }

    status = EXIT_SUCCESS;

cleanup:
    alarm(0);

    if (close(fd) == -1) {
        perror("close");
        status = EXIT_FAILURE;
    }

    return status;
}