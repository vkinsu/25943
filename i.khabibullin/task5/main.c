#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

//  для хранения информации о строке
typedef struct
{
    off_t offset;  // Смещение от начала файла
    size_t length; // Длина строки, включая символ перевода строки
} LineInfo;

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

    while ((bytes_read = read(fd, &ch, 1)) > 0)
    {
        if (ch == '\n')
        {
            // Текущая позиция в файле с помощью lseek(fd, 0L, 1)
            off_t current_pos = lseek(fd, 0L, SEEK_CUR);

            // Расширяем массив при необходимости
            if (line_count >= capacity)
            {
                capacity *= 2;
                table = realloc(table, capacity * sizeof(LineInfo));
            }

            // Записываем данные о строке
            table[line_count].offset = current_line_start;
            table[line_count].length = (size_t)(current_pos - current_line_start);
            line_count++;

            // Следующая строка начнется сразу за символом '\n'
            current_line_start = current_pos;
        }
    }

    // если файл не заканчивается символом перевода строки
    off_t total_size = lseek(fd, 0L, SEEK_END);
    if (total_size > current_line_start)
    {
        if (line_count >= capacity)
        {
            capacity += 1;
            table = realloc(table, capacity * sizeof(LineInfo));
        }
        table[line_count].offset = current_line_start;
        table[line_count].length = (size_t)(total_size - current_line_start);
        line_count++;
    }

    printf("\nТАБЛИЦА ИНДЕКСАЦИИ СТРОК\n");
    printf("%-10s %-10s %-10s\n", "№ Строки", "Отступ", "Длина");
    for (size_t i = 0; i < line_count; i++)
    {
        printf("%-10zu %-10lld %-10zu\n", i + 1, (long long)table[i].offset, table[i].length);
    }
    printf("================================\n\n");

    // запрос номера строки
    int target_line;
    while (1)
    {
        printf("Введите номер строки (1..%zu) или 0 для выхода: ", line_count);
        if (scanf("%d", &target_line) != 1)
        {
            printf("Некорректный ввод.\n");
            while (getchar() != '\n')
                ;
            continue;
        }

        if (target_line == 0)
        {
            printf("Завершение работы.\n");
            break;
        }

        if (target_line < 1 || (size_t)target_line > line_count)
        {
            int target_line;
            printf("Ошибка: Строка с таким номером отсутствует в файле\n");
            continue;
        }

        size_t idx = (size_t)target_line - 1;

        lseek(fd, table[idx].offset, SEEK_SET);

        char *buffer = malloc(table[idx].length + 1);
        if (!buffer)
        {
            perror("Ошибка при чтении строки");
            break;
        }

        ssize_t read_bytes = read(fd, buffer, table[idx].length);
        if (read_bytes > 0)
        {
            buffer[read_bytes] = '\0';
            printf("Строка %d: %s", target_line, buffer);

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

    free(table);
    close(fd);
    return 0;
}
