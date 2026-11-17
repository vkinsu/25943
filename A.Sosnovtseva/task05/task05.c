#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>

typedef struct {
    off_t offset;
    size_t length;
} Line;

static ssize_t read_retry(int fd, void *buf, size_t size)
{
    ssize_t n;
    do {
        n = read(fd, buf, size);
    } while (n < 0 && errno == EINTR);
    return n;
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Использование: %s файл.txt\n", argv[0]);
        return EXIT_FAILURE;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        return EXIT_FAILURE;
    }

    Line *table = NULL;
    size_t count = 0, length = 0;
    off_t start = 0;
    char *text = NULL;
    int result = EXIT_FAILURE;

    for (;;) {
        char c;
        ssize_t n = read_retry(fd, &c, 1);

        if (n < 0) {
            perror("read");
            goto cleanup;
        }
        if (n == 0 && length == 0)
            break;

        if (n > 0) {
            if (length == SIZE_MAX - 1)
                goto too_large;
            ++length;
            if (c != '\n')
                continue;
        }

        off_t end = lseek(fd, 0L, SEEK_CUR);
        if (end == (off_t)-1) {
            perror("lseek");
            goto cleanup;
        }

        if (count >= SIZE_MAX / sizeof(*table))
            goto too_large;

        Line *tmp = realloc(table, (count + 1) * sizeof(*table));
        if (!tmp) {
            perror("realloc");
            goto cleanup;
        }

        table = tmp;
        table[count++] = (Line){start, length};
        start = end;
        length = 0;

        if (n == 0)
            break;
    }

    printf("Номер\tСмещение\tДлина (байт)\n");
    for (size_t i = 0; i < count; ++i) {
        printf("%zu\t%jd\t\t%zu\n",
               i + 1, (intmax_t)table[i].offset, table[i].length);
    }

    for (;;) {
        char input[128], *end;

        printf("Номер строки (0 — выход): ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin))
            break;

        if (!strchr(input, '\n') && !feof(stdin)) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF) {}
            puts("Слишком длинный ввод.");
            continue;
        }

        errno = 0;
        intmax_t number = strtoimax(input, &end, 10);
        int invalid = (end == input || errno == ERANGE);

        while (isspace((unsigned char)*end))
            ++end;

        if (invalid || *end || number < 0 ||
            (uintmax_t)number > count) {
            puts("Неверный номер строки.");
            continue;
        }

        if (number == 0)
            break;

        Line line = table[(size_t)number - 1];

        if (lseek(fd, line.offset, SEEK_SET) == (off_t)-1) {
            perror("lseek");
            goto cleanup;
        }

        text = malloc(line.length + 1);
        if (!text) {
            perror("malloc");
            goto cleanup;
        }

        size_t total = 0;
        while (total < line.length) {
            size_t chunk = line.length - total;
            if (chunk > 4096)
                chunk = 4096;

            ssize_t n = read_retry(fd, text + total, chunk);
            if (n < 0) {
                perror("read");
                goto cleanup;
            }
            if (n == 0) {
                fprintf(stderr, "Неожиданный конец файла.\n");
                goto cleanup;
            }

            total += (size_t)n;
        }

        text[line.length] = '\0';
        printf("%s", text);

        if (text[line.length - 1] != '\n')
            printf("\n");

        free(text);
        text = NULL;
    }

    result = EXIT_SUCCESS;
    goto cleanup;

too_large:
    fprintf(stderr, "Файл слишком большой.\n");

cleanup:
    free(text);
    free(table);

    if (close(fd) < 0) {
        perror("close");
        result = EXIT_FAILURE;
    }

    return result;
}