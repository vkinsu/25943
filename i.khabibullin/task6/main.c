#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

typedef struct
{
    off_t offset;
    size_t length;
} LineInfo;

/* Флаг срабатывания таймера */
volatile sig_atomic_t time_is_up = 0;

/* Обработчик SIGALRM */
void alarm_handler(int sig)
{
    (void)sig;
    time_is_up = 1;
}

/* Печать всего файла */
void print_file(int fd)
{
    char buffer[1024];
    ssize_t bytes_read;

    /* Переходим в начало файла */
    lseek(fd, 0, SEEK_SET);

    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
    {
        write(STDOUT_FILENO, buffer, bytes_read);
    }
}

int main(void)
{
    char filename[256];

    printf("Введите имя текстового файла: ");

    if (scanf("%255s", filename) != 1)
    {
        return 1;
    }

    /* Открываем файл */
    int fd = open(filename, O_RDONLY);

    if (fd < 0)
    {
        perror("Ошибка открытия файла");
        return 1;
    }

    /* Создаём таблицу строк */
    size_t capacity = 10;
    size_t line_count = 0;

    LineInfo *table = malloc(capacity * sizeof(LineInfo));

    if (!table)
    {
        perror("Ошибка выделения памяти");
        close(fd);
        return 1;
    }

    off_t current_line_start = 0;
    char ch;
    ssize_t bytes_read;

    /* Строим таблицу строк */
    while ((bytes_read = read(fd, &ch, 1)) > 0)
    {
        if (ch == '\n')
        {
            off_t current_pos = lseek(fd, 0, SEEK_CUR);

            /* Если места в таблице не хватает */
            if (line_count >= capacity)
            {
                capacity *= 2;

                LineInfo *tmp =
                    realloc(table, capacity * sizeof(LineInfo));

                if (!tmp)
                {
                    perror("Ошибка realloc");
                    free(table);
                    close(fd);
                    return 1;
                }

                table = tmp;
            }

            table[line_count].offset = current_line_start;

            table[line_count].length =
                (size_t)(current_pos - current_line_start);

            line_count++;

            current_line_start = current_pos;
        }
    }

    /* Если последняя строка не заканчивается '\n' */
    off_t total_size = lseek(fd, 0, SEEK_END);

    if (total_size > current_line_start)
    {
        if (line_count >= capacity)
        {
            capacity *= 2;

            LineInfo *tmp =
                realloc(table, capacity * sizeof(LineInfo));

            if (!tmp)
            {
                perror("Ошибка realloc");
                free(table);
                close(fd);
                return 1;
            }

            table = tmp;
        }

        table[line_count].offset = current_line_start;

        table[line_count].length =
            (size_t)(total_size - current_line_start);

        line_count++;
    }

    /* Выводим таблицу */
    printf("\nТАБЛИЦА ИНДЕКСАЦИИ СТРОК\n");

    printf("%-10s %-10s %-10s\n",
           "№ Строки",
           "Отступ",
           "Длина");

    for (size_t i = 0; i < line_count; i++)
    {
        printf("%-10zu %-10lld %-10zu\n",
               i + 1,
               (long long)table[i].offset,
               table[i].length);
    }

    printf("================================\n\n");

    /* Настраиваем обработчик SIGALRM */
    struct sigaction sa = {0};

    sa.sa_handler = alarm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGALRM, &sa, NULL) == -1)
    {
        perror("Ошибка sigaction");
        free(table);
        close(fd);
        return 1;
    }

    int target_line;

    int first_input = 1;

    while (1)
    {
        printf(
            "Введите номер строки (1..%zu) или 0 для выхода",
            line_count);

        if (first_input)
        {
            printf(" (у вас 5 секунд)");
        }

        printf(": ");
        fflush(stdout);

        /*
         * Таймер запускаем толкьо перед первым вводом.
         */
        if (first_input)
        {
            time_is_up = 0;
            alarm(5);
        }

        int result = scanf("%d", &target_line);

        /*
         * Этот блок выполнится только после первого scanf.
         */
        if (first_input)
        {
            /* Отменяем таймер */
            alarm(0);

            /*
             * Если за время ожидания пришёл SIGALRM,
             * выводим весь файл и завершаем программу.
             */
            if (time_is_up)
            {
                printf("\nВремя вышло!\n");
                printf("Содержимое файла:\n");
                printf("============================\n");

                print_file(fd);

                printf("\n============================\n");

                break;
            }

            /*
             * Первый ввод произошёл.
             */
            first_input = 0;
        }

        /* Проверяем корректность ввода */
        if (result != 1)
        {
            printf("Некорректный ввод.\n");

            int c;

            while ((c = getchar()) != '\n' && c != EOF)
            {
                /* очищаем stdin */
            }

            continue;
        }

        /* 0 — завершение программы */
        if (target_line == 0)
        {
            printf("Завершение работы.\n");
            break;
        }

        /* Проверяем существование строки */
        if (target_line < 1 ||
            (size_t)target_line > line_count)
        {
            printf(
                "Ошибка: строка с таким номером отсутствует\n");

            continue;
        }

        /* Индекс строки в таблице */
        size_t idx = (size_t)target_line - 1;

        /* Переходим к началу нужной строки */
        if (lseek(fd, table[idx].offset, SEEK_SET) == -1)
        {
            perror("Ошибка lseek");
            break;
        }

        /* Выделяем память под строку */
        char *buffer = malloc(table[idx].length + 1);

        if (!buffer)
        {
            perror("Ошибка выделения памяти");
            break;
        }

        /* Читаем нужную строку */
        ssize_t read_bytes =
            read(fd, buffer, table[idx].length);

        if (read_bytes > 0)
        {
            /* Делаем обычную C-строку */
            buffer[read_bytes] = '\0';

            printf(
                "Строка %d: %s",
                target_line,
                buffer);

            /*
             * Если в файле строка не закончилась '\n'
             * добавляем его для красивого вывода
             */
            if (buffer[read_bytes - 1] != '\n')
            {
                printf("\n");
            }
        }
        else
        {
            printf("Ошибка чтения строки из файла.\n");
        }

        free(buffer);
    }

    /* Освобождаем ресурсы */
    free(table);
    close(fd);

    return 0;
}