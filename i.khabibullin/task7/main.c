#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>

typedef struct
{
    off_t offset;
    size_t length;
} LineInfo;

volatile sig_atomic_t time_is_up = 0;

void alarm_handler(int sig)
{
    (void)sig;
    time_is_up = 1;
}

int main(void)
{
    char filename[256];

    printf("Введите имя текстового файла: ");

    if (scanf("%255s", filename) != 1)
    {
        return 1;
    }

    int fd = open(filename, O_RDONLY);

    if (fd < 0)
    {
        perror("Ошибка открытия файла");
        return 1;
    }

    /*
     * Узнаём размер файла.
     */
    struct stat st;

    if (fstat(fd, &st) == -1)
    {
        perror("fstat");
        close(fd);
        return 1;
    }

    size_t file_size = (size_t)st.st_size;

    if (file_size == 0)
    {
        printf("Файл пуст.\n");
        close(fd);
        return 0;
    }

    /*
     * Отображаем файл в память
     */
    char *data = mmap(
        NULL,
        file_size,
        PROT_READ,
        MAP_PRIVATE,
        fd,
        0);

    if (data == MAP_FAILED)
    {
        perror("mmap");
        close(fd);
        return 1;
    }

    /*
     * После mmap дескриптор можно закрыть
     */
    close(fd);

    size_t capacity = 10;
    size_t line_count = 0;

    LineInfo *table =
        malloc(capacity * sizeof(LineInfo));

    if (!table)
    {
        perror("malloc");
        munmap(data, file_size);
        return 1;
    }

    /*
     * Строим таблицу строк.
     *
     */
    size_t current_line_start = 0;

    for (size_t i = 0; i < file_size; i++)
    {
        if (data[i] == '\n')
        {
            if (line_count >= capacity)
            {
                capacity *= 2;

                LineInfo *tmp =
                    realloc(table,
                            capacity * sizeof(LineInfo));

                if (!tmp)
                {
                    perror("realloc");
                    free(table);
                    munmap(data, file_size);
                    return 1;
                }

                table = tmp;
            }

            table[line_count].offset =
                (off_t)current_line_start;

            table[line_count].length =
                i - current_line_start + 1;

            line_count++;

            current_line_start = i + 1;
        }
    }

    /*
     * Последняя строка без '\n'
     */
    if (current_line_start < file_size)
    {
        if (line_count >= capacity)
        {
            capacity *= 2;

            LineInfo *tmp =
                realloc(table,
                        capacity * sizeof(LineInfo));

            if (!tmp)
            {
                perror("realloc");
                free(table);
                munmap(data, file_size);
                return 1;
            }

            table = tmp;
        }

        table[line_count].offset =
            (off_t)current_line_start;

        table[line_count].length =
            file_size - current_line_start;

        line_count++;
    }

    printf("\nТАБЛИЦА ИНДЕКСАЦИИ СТРОК\n");

    printf("%-10s %-10s %-10s\n",
           "№ Строки", "Отступ", "Длина");

    for (size_t i = 0; i < line_count; i++)
    {
        printf("%-10zu %-10lld %-10zu\n",
               i + 1,
               (long long)table[i].offset,
               table[i].length);
    }

    printf("================================\n\n");

    /*
     * Настраиваем alarm
     */
    struct sigaction sa = {0};

    sa.sa_handler = alarm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGALRM, &sa, NULL) == -1)
    {
        perror("sigaction");

        free(table);
        munmap(data, file_size);

        return 1;
    }

    int target_line;

    while (1)
    {
        time_is_up = 0;

        printf("Введите номер строки (1..%zu) или 0 для выхода "
               "(у вас 5 секунд): ",
               line_count);

        fflush(stdout);

        alarm(5);

        int result = scanf("%d", &target_line);

        alarm(0);

        /*
         * Время закончилось
         */
        if (time_is_up)
        {
            printf("\n\nВремя вышло\n");
            printf("Содержимое файла:\n");
            printf("============================\n");

            for (size_t i = 0; i < file_size; i++)
            {
                putchar(data[i]);
            }

            if (data[file_size - 1] != '\n')
            {
                putchar('\n');
            }

            break;
        }

        if (result != 1)
        {
            printf("Некорректный ввод\n");

            int c;
            while ((c = getchar()) != '\n' && c != EOF)
                ;

            continue;
        }

        if (target_line == 0)
        {
            printf("Завершение работы\n");
            break;
        }

        if (target_line < 1 ||
            (size_t)target_line > line_count)
        {
            printf("Ошибка: строка с таким номером отсутствует\n");
            continue;
        }

        size_t idx = (size_t)target_line - 1;

        char *line =
            data + table[idx].offset;

        printf("Строка %d: ", target_line);

        for (size_t i = 0;
             i < table[idx].length;
             i++)
        {
            putchar(line[i]);
        }

        if (line[table[idx].length - 1] != '\n')
        {
            putchar('\n');
        }
    }

    free(table);

    if (munmap(data, file_size) == -1)
    {
        perror("munmap");
        return 1;
    }

    return 0;
}