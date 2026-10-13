#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#define MAX_LINES 100

struct Line
{
    off_t offset;
    size_t length;
};

volatile sig_atomic_t time_is_up = 0;

void alarm_handler(int signal_number)
{
    (void)signal_number;
    time_is_up = 1;
}

int main(int argc, char *argv[])
{
    int fd;
    char ch;
    struct Line lines[MAX_LINES];
    int line_count = 0;

    off_t line_start = 0;
    off_t current_position;
    ssize_t bytes_read;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s filename\n", argv[0]);
        return 1;
    }

    fd = open(argv[1], O_RDONLY);

    if (fd == -1)
    {
        perror("open");
        return 1;
    }

    while ((bytes_read = read(fd, &ch, 1)) > 0)
    {
        if (ch == '\n')
        {
            if (line_count >= MAX_LINES)
            {
                fprintf(stderr, "Too many lines\n");
                close(fd);
                return 1;
            }

            current_position = lseek(fd, 0L, 1);

            if (current_position == (off_t)-1)
            {
                perror("lseek");
                close(fd);
                return 1;
            }

            lines[line_count].offset = line_start;
            lines[line_count].length =
                current_position - line_start;

            line_count++;
            line_start = current_position;
        }
    }

    if (bytes_read == -1)
    {
        perror("read");
        close(fd);
        return 1;
    }

    current_position = lseek(fd, 0L, 1);

    if (current_position == (off_t)-1)
    {
        perror("lseek");
        close(fd);
        return 1;
    }

    if (line_start < current_position)
    {
        if (line_count >= MAX_LINES)
        {
            fprintf(stderr, "Too many lines\n");
            close(fd);
            return 1;
        }

        lines[line_count].offset = line_start;
        lines[line_count].length =
            current_position - line_start;

        line_count++;
    }

    if (signal(SIGALRM, alarm_handler) == SIG_ERR)
    {
        perror("signal");
        close(fd);
        return 1;
    }

    printf("Таблица строк:\n\n");
    printf("№\tОтступ\tДлина\n");

    for (int i = 0; i < line_count; i++)
    {
        printf("%d\t%ld\t%lu\n",
               i + 1,
               (long)lines[i].offset,
               (unsigned long)lines[i].length);
    }

    while (1)
    {
        int number;

        printf("\nВведите номер строки (0 - выход): ");
        fflush(stdout);

        time_is_up = 0;
        alarm(5);

        if (scanf("%d", &number) != 1)
        {
            alarm(0);

            if (time_is_up)
            {
                break;
            }

            fprintf(stderr, "Некорректный номер строки\n");
            close(fd);
            return 1;
        }

        alarm(0);

        if (number == 0)
        {
            close(fd);
            return 0;
        }

        if (number < 1 || number > line_count)
        {
            printf("Такой строки нет\n");
            continue;
        }

        if (lseek(fd, lines[number - 1].offset, SEEK_SET) == -1)
        {
            perror("lseek");
            close(fd);
            return 1;
        }

        size_t length = lines[number - 1].length;

        char *line = malloc(length + 1);

        if (line == NULL)
        {
            perror("malloc");
            close(fd);
            return 1;
        }

        ssize_t n = read(fd, line, length);

        if (n == -1)
        {
            perror("read");
            free(line);
            close(fd);
            return 1;
        }

        line[n] = '\0';

        printf("%s", line);

        free(line);
    }

    printf("\nВремя истекло. Содержимое файла:\n\n");

    if (lseek(fd, 0L, SEEK_SET) == -1)
    {
        perror("lseek");
        close(fd);
        return 1;
    }

    while ((bytes_read = read(fd, &ch, 1)) > 0)
    {
        printf("%c", ch);
    }

    if (bytes_read == -1)
    {
        perror("read");
        close(fd);
        return 1;
    }

    if (close(fd) == -1)
    {
        perror("close");
        return 1;
    }

    return 0;
}