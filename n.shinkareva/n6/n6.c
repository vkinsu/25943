#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>

// информация о расположении строки в файле
typedef struct {
    off_t offset;
    size_t length;
} LineInfo;

// флаг срабатывания таймера
volatile sig_atomic_t time_is_up = 0;

// обработчик сигнала от alarm
void alarm_handler(int signal_number)
{
    (void)signal_number;
    time_is_up = 1;
}

// считываем ввод, при необходимости ограничивая время
int read_input(char *text, size_t capacity, int use_timer)
{
    size_t length = 0;
    int too_long = 0;

    if (use_timer) {
        time_is_up = 0;
        alarm(5);
    }

    while (1) {
        char symbol;
        ssize_t result = read(STDIN_FILENO, &symbol, 1);

        // проверяем, не истекли ли пять секунд
        if (use_timer && time_is_up) {
            alarm(0);
            return -2;
        }

        if (result == 0) {
            if (length == 0 && !too_long) {
                if (use_timer)
                    alarm(0);
                return 0;
            }
            break;
        }

        if (result == -1) {
            if (errno == EINTR)
                continue;

            if (use_timer)
                alarm(0);

            return -1;
        }

        if (symbol == '\n')
            break;

        // защищаем буфер от переполнения
        if (length + 1 < capacity)
            text[length++] = symbol;
        else
            too_long = 1;
    }

    // отменяем таймер после завершения ввода
    if (use_timer) {
        alarm(0);

        if (time_is_up)
            return -2;
    }

    text[length] = '\0';

    return too_long ? -3 : 1;
}

// добавляем новую строку в таблицу
int add_line(LineInfo **table, size_t *count,
             size_t *capacity, off_t start, off_t end)
{
    if (end < start ||
        (uintmax_t)(end - start) > SIZE_MAX)
        return -1;

    if (*count == *capacity) {
        size_t new_capacity =
            (*capacity == 0) ? 16 : *capacity * 2;

        LineInfo *new_table;

        if (new_capacity < *capacity ||
            new_capacity > SIZE_MAX / sizeof(LineInfo))
            return -1;

        new_table = realloc(
            *table, new_capacity * sizeof(LineInfo)
        );

        if (new_table == NULL)
            return -1;

        *table = new_table;
        *capacity = new_capacity;
    }

    (*table)[*count].offset = start;
    (*table)[*count].length = (size_t)(end - start);

    (*count)++;
    return 0;
}

// строим таблицу смещений и длин строк
int build_table(int file, LineInfo **table, size_t *count)
{
    size_t capacity = 0;
    off_t line_start = 0;
    off_t position;
    char symbol;
    ssize_t result;

    while ((result = read(file, &symbol, 1)) == 1) {
        if (symbol == '\n') {
            position = lseek(file, 0, SEEK_CUR);

            if (position == (off_t)-1 ||
                add_line(table, count, &capacity,
                         line_start, position) == -1)
                return -1;

            line_start = position;
        }
    }

    if (result == -1)
        return -1;

    // учитываем последнюю строку без перевода строки
    position = lseek(file, 0, SEEK_CUR);

    if (position == (off_t)-1)
        return -1;

    if (position > line_start &&
        add_line(table, count, &capacity,
                 line_start, position) == -1)
        return -1;

    return 0;
}

// выводим указанную часть файла
int print_part(int file, off_t offset, size_t length)
{
    char buffer[1024];

    if (lseek(file, offset, SEEK_SET) == (off_t)-1)
        return -1;

    while (length > 0) {
        size_t wanted = length < sizeof(buffer)
                            ? length : sizeof(buffer);

        ssize_t bytes_read = read(file, buffer, wanted);
        size_t written = 0;

        if (bytes_read < 0 && errno == EINTR)
            continue;

        if (bytes_read <= 0)
            return -1;

        // учитываем возможность неполной записи
        while (written < (size_t)bytes_read) {
            ssize_t n = write(
                STDOUT_FILENO,
                buffer + written,
                (size_t)bytes_read - written
            );

            if (n < 0 && errno == EINTR)
                continue;

            if (n <= 0)
                return -1;

            written += (size_t)n;
        }

        length -= (size_t)bytes_read;
    }

    return 0;
}

// выводим весь файл, если время истекло
int print_whole_file(int file)
{
    off_t file_size = lseek(file, 0, SEEK_END);

    if (file_size == (off_t)-1 ||
        (uintmax_t)file_size > SIZE_MAX)
        return -1;

    return print_part(file, 0, (size_t)file_size);
}

int main(void)
{
    char filename[256];
    char input[128];

    LineInfo *table = NULL;

    size_t line_count = 0;
    size_t i;

    int file;
    int status;
    int first_input = 1;
    int exit_code = 0;

    long line_number;
    char *end;

    struct sigaction action = {0};

    // запрашиваем имя файла
    printf("введите имя текстового файла: ");
    fflush(stdout);

    status = read_input(filename, sizeof(filename), 0);

    if (status != 1 || filename[0] == '\0') {
        printf("неверное имя файла\n");
        return 1;
    }

    // открываем файл для чтения
    file = open(filename, O_RDONLY);

    if (file == -1) {
        perror("open");
        return 1;
    }

    // строим таблицу строк
    if (build_table(file, &table, &line_count) == -1) {
        perror("ошибка построения таблицы");
        free(table);
        close(file);
        return 1;
    }

    // выводим таблицу для проверки
    printf("\nномер  смещение  длина\n");

    for (i = 0; i < line_count; i++) {
        printf("%-6zu %-9lld %zu\n",
               i + 1,
               (long long)table[i].offset,
               table[i].length);
    }

    // настраиваем обработчик сигнала SIGALRM
    action.sa_handler = alarm_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if (sigaction(SIGALRM, &action, NULL) == -1) {
        perror("sigaction");
        free(table);
        close(file);
        return 1;
    }

    // запрашиваем номера строк
    while (1) {
        printf("\nномер строки (0 - выход)%s: ",
               first_input ? " [5 секунд]" : "");

        fflush(stdout);

        // ограничиваем время только первого ввода
        status = read_input(input, sizeof(input), first_input);
        first_input = 0;

        // если пользователь не успел, выводим весь файл
        if (status == -2) {
            printf("\nвремя вышло! содержимое файла:\n");
            fflush(stdout);

            if (print_whole_file(file) == -1) {
                perror("ошибка печати файла");
                exit_code = 1;
            }

            printf("\n");
            break;
        }

        // обрабатываем конец ввода
        if (status == 0)
            break;

        if (status == -1) {
            perror("read");
            exit_code = 1;
            break;
        }

        if (status == -3) {
            printf("слишком длинный номер строки\n");
            continue;
        }

        // преобразуем введенный номер в число
        errno = 0;
        line_number = strtol(input, &end, 10);

        // пропускаем пробелы после номера
        while (*end == ' ' || *end == '\t' || *end == '\r')
            end++;

        // проверяем правильность номера
        if (end == input || *end != '\0' ||
            errno == ERANGE || line_number < 0) {

            printf("неверный номер строки\n");
            continue;
        }

        // ноль завершает программу
        if (line_number == 0)
            break;

        // проверяем существование строки
        if ((uintmax_t)line_number > line_count) {
            printf("такой строки нет\n");
            continue;
        }

        // находим строку в таблице
        LineInfo selected = table[line_number - 1];

        printf("строка %ld: ", line_number);
        fflush(stdout);

        // переходим к строке и выводим ее
        if (print_part(file, selected.offset,
                       selected.length) == -1) {

            perror("ошибка чтения строки");
            exit_code = 1;
            break;
        }

        // добавляем перенос, если его нет в конце строки
        if (selected.length == 0) {
            printf("\n");
        }
        else {
            char last_symbol;

            if (lseek(file,
                      selected.offset +
                      (off_t)selected.length - 1,
                      SEEK_SET) != (off_t)-1 &&
                read(file, &last_symbol, 1) == 1 &&
                last_symbol != '\n') {

                printf("\n");
            }
        }
    }

    // освобождаем память и закрываем файл
    free(table);

    if (close(file) == -1) {
        perror("close");
        exit_code = 1;
    }

    return exit_code;
}