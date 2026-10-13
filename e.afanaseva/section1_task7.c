#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>

#define MAX_LINES 100

struct Line
{
    size_t offset;
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
    struct stat file_info;
    char *data = NULL;
    size_t file_size;

    struct Line lines[MAX_LINES];
    int line_count = 0;
    size_t line_start = 0;

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

    if (fstat(fd, &file_info) == -1)
    {
        perror("fstat");
        close(fd);
        return 1;
    }

    if (file_info.st_size < 0)
    {
        fprintf(stderr, "Invalid file size\n");
        close(fd);
        return 1;
    }

    file_size = (size_t)file_info.st_size;

    if (file_size > 0)
    {
        data = mmap(NULL, file_size,
                    PROT_READ, MAP_PRIVATE, fd, 0);

        if (data == MAP_FAILED)
        {
            perror("mmap");
            close(fd);
            return 1;
        }
    }

    close(fd);

    for (size_t i = 0; i < file_size; i++)
    {
        if (data[i] == '\n')
        {
            if (line_count >= MAX_LINES)
            {
                fprintf(stderr, "Too many lines\n");
                goto cleanup_error;
            }

            lines[line_count].offset = line_start;
            lines[line_count].length = i - line_start + 1;

            line_count++;
            line_start = i + 1;
        }
    }

    if (line_start < file_size)
    {
        if (line_count >= MAX_LINES)
        {
            fprintf(stderr, "Too many lines\n");
            goto cleanup_error;
        }

        lines[line_count].offset = line_start;
        lines[line_count].length = file_size - line_start;

        line_count++;
    }

    if (signal(SIGALRM, alarm_handler) == SIG_ERR)
    {
        perror("signal");
        goto cleanup_error;
    }

    printf("Таблица строк:\n\n");
    printf("№\tОтступ\tДлина\n");

    for (int i = 0; i < line_count; i++)
    {
        printf("%d\t%lu\t%lu\n",
               i + 1,
               (unsigned long)lines[i].offset,
               (unsigned long)lines[i].length);
    }

    while (1)
    {
        int number;

        printf("\nВведите номер строки (0 - выход): ");
        fflush(stdout);

        time_is_up = 0;
        alarm(5);

        int result = scanf("%d", &number);

        alarm(0);

        if (time_is_up)
        {
            break;
        }

        if (result != 1)
        {
            fprintf(stderr, "Некорректный номер строки\n");
            goto cleanup_error;
        }

        if (number == 0)
        {
            break;
        }

        if (number < 1 || number > line_count)
        {
            printf("Такой строки нет\n");
            continue;
        }

        size_t offset = lines[number - 1].offset;
        size_t length = lines[number - 1].length;

        printf("%.*s", (int)length, data + offset);

        if (length == 0 || data[offset + length - 1] != '\n')
        {
            putchar('\n');
        }
    }

    if (time_is_up)
    {
        printf("\nВремя истекло. Содержимое файла:\n\n");

        if (file_size > 0)
        {
            if (fwrite(data, 1, file_size, stdout) != file_size)
            {
                perror("fwrite");
                goto cleanup_error;
            }
        }
    }

    if (file_size > 0)
    {
        if (munmap(data, file_size) == -1)
        {
            perror("munmap");
            return 1;
        }
    }

    return 0;

cleanup_error:
    if (file_size > 0)
    {
        munmap(data, file_size);
    }

    return 1;
}