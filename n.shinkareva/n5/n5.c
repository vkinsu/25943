
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

// храним смещение строки в файле и ее длину в байтах
typedef struct {
    off_t offset;
    size_t length;
} LineInfo;

// добавляем строку в таблицу, при необходимости увеличиваем ее размер
int add_line(LineInfo **table, size_t *count, size_t *capacity,
             off_t start, off_t end)
{
    if (end < start || (uintmax_t)(end - start) > SIZE_MAX)
        return 0;

    if (*count == *capacity) {
        size_t new_capacity = (*capacity == 0) ? 16 : (*capacity * 2);
        LineInfo *new_table;

        if (new_capacity < *capacity ||
            new_capacity > SIZE_MAX / sizeof(LineInfo))
            return 0;

        new_table = realloc(*table, new_capacity * sizeof(LineInfo));
        if (new_table == NULL)
            return 0;

        *table = new_table;
        *capacity = new_capacity;
    }

    (*table)[*count].offset = start;
    (*table)[*count].length = (size_t)(end - start);
    (*count)++;
    return 1;
}

// строим таблицу, находя символы перевода строки
int build_table(int file, LineInfo **table, size_t *count)
{
    size_t capacity = 0;
    off_t line_start = 0;
    off_t position;
    char symbol;
    ssize_t bytes_read;

    while ((bytes_read = read(file, &symbol, 1)) == 1) {
        if (symbol == '\n') {
            // узнаем текущую позицию после прочитанного символа
            position = lseek(file, 0L, SEEK_CUR);

            if (position == (off_t)-1 ||
                !add_line(table, count, &capacity, line_start, position))
                return -1;

            line_start = position;
        }
    }

    if (bytes_read == -1)
        return -1;

    // учитываем последнюю строку, даже если она без \n
    position = lseek(file, 0L, SEEK_CUR);

    if (position == (off_t)-1)
        return -1;

    if (position > line_start &&
        !add_line(table, count, &capacity, line_start, position))
        return -1;

    return 0;
}

// переходим к нужной строке и читаем ровно ее длину
int print_line(int file, LineInfo info)
{
    char *text;
    size_t total = 0;
    ssize_t bytes_read;

    if (info.length == SIZE_MAX)
        return -1;

    text = malloc(info.length + 1);
    if (text == NULL)
        return -1;

    if (lseek(file, info.offset, SEEK_SET) == (off_t)-1) {
        free(text);
        return -1;
    }

    // читаем строку полностью, даже если read вернул только часть
    while (total < info.length) {
        bytes_read = read(file, text + total, info.length - total);

        if (bytes_read <= 0) {
            free(text);
            return -1;
        }

        total += (size_t)bytes_read;
    }

    text[total] = '\0';

    printf("%s", text);

    // добавляем перенос, если его не было в файле
    if (total == 0 || text[total - 1] != '\n')
        printf("\n");

    free(text);
    return 0;
}

int main(void)
{
    char filename[256];
    char input[128];

    LineInfo *table = NULL;

    size_t line_count = 0;
    size_t i;

    int file;
    int result = 0;

    long number;
    char *end;

    // запрашиваем имя текстового файла
    printf("введите имя файла: ");

    if (fgets(filename, sizeof(filename), stdin) == NULL)
        return 1;

    // убираем перевод строки из имени файла
    filename[strcspn(filename, "\n")] = '\0';

    // открываем файл только для чтения
    file = open(filename, O_RDONLY);

    if (file == -1) {
        perror("open");
        return 1;
    }

    // строим таблицу смещений и длин строк
    if (build_table(file, &table, &line_count) == -1) {
        perror("ошибка чтения или выделения памяти");
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

    // запрашиваем номера строк до ввода нуля
    while (1) {
        printf("\nномер строки (0 - выход): ");

        if (fgets(input, sizeof(input), stdin) == NULL)
            break;

        // очищаем остаток слишком длинного ввода
        if (strchr(input, '\n') == NULL && !feof(stdin)) {
            int symbol;

            while ((symbol = getchar()) != '\n' && symbol != EOF) {
            }

            printf("неверный номер строки\n");
            continue;
        }

        // преобразуем введенную строку в число
        errno = 0;
        number = strtol(input, &end, 10);

        // пропускаем пробелы после числа
        while (isspace((unsigned char)*end))
            end++;

        // проверяем неправильный ввод
        if (end == input || *end != '\0' ||
            errno == ERANGE || number < 0) {

            printf("неверный номер строки\n");
            continue;
        }

        // ноль завершает программу
        if (number == 0)
            break;

        // проверяем существование строки
        if ((uintmax_t)number > line_count) {
            printf("такой строки нет\n");
            continue;
        }

        // переходим к строке и выводим ее
        printf("строка %ld: ", number);

        if (print_line(file, table[number - 1]) == -1) {
            perror("ошибка чтения строки");
            result = 1;
            break;
        }
    }

    // освобождаем память и закрываем файл
    free(table);

    if (close(file) == -1) {
        perror("close");
        return 1;
    }

    return result;
}
