#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define INITIAL_CAPACITY 16
#define BUFFER_SIZE 256

// cтруктура для хранения отступа и длины строки
typedef struct {
    off_t offset;
    size_t length;  // длина строки в байтах
} LineInfo;

int main(int argc, char *argv[]) 
{
    if (argc < 2) 
    {
        fprintf(stderr, "Использование: %s <имя_файла>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) 
    {
        perror("Ошибка открытия файла");
        return EXIT_FAILURE;
    }

    // динамический массив для таблицы строк
    size_t capacity = INITIAL_CAPACITY;
    size_t line_count = 0;
    LineInfo *table = malloc(capacity * sizeof(LineInfo));
    if (!table) 
    {
        perror("Ошибка выделения памяти");
        close(fd);
        return EXIT_FAILURE;
    }

    // первая строка начинается с отступа 0
    table[0].offset = 0;
    table[0].length = 0;

    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    // считываем файл блоками и ищем символы перевода строки '\n'
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) 
    {
        for (ssize_t i = 0; i < bytes_read; i++) 
        {
            table[line_count].length++;

            if (buffer[i] == '\n') 
            {
                // текущая позиция через lseek(fd, 0L, 1)
                // lseek указывает на следующий за сбалансированным буфером символ с учетом прочитанного
                off_t current_pos = lseek(fd, 0L, SEEK_CUR) - bytes_read + i + 1;

                line_count++;

                // расширяем таблицу при необходимости
                if (line_count >= capacity) 
                {
                    capacity *= 2;
                    LineInfo *temp = realloc(table, capacity * sizeof(LineInfo));
                    if (!temp) 
                    {
                        perror("Ошибка перераспределения памяти");
                        free(table);
                        close(fd);
                        return EXIT_FAILURE;
                    }
                    table = temp;
                }

                // следующая строка начинается сразу после '\n'
                table[line_count].offset = current_pos;
                table[line_count].length = 0;
            }
        }
    }

    if (bytes_read == -1) 
    {
        perror("Ошибка чтения файла");
        free(table);
        close(fd);
        return EXIT_FAILURE;
    }

    // если файл не заканчивается символом '\n', учтем последнюю строку
    if (table[line_count].length > 0) 
    {
        line_count++;
    }

    // отладочный вывод таблицы отступов и длин
    printf("=== Таблица отступов и длин строк ===\n");
    printf("%-8s | %-12s | %-10s\n", "Строка", "Смещение (B)", "Длина (B)");
    for (size_t i = 0; i < line_count; i++) 
    {
        printf("%-8zu | %-12ld | %-10zu\n", i + 1, (long)table[i].offset, table[i].length);
    }
    printf("=====================================\n\n");

    // цикл запроса номера строки
    int line_num;
    while (1) 
    {
        printf("Введите номер строки (1-%zu, 0 для выхода): ", line_count);
        if (scanf("%d", &line_num) != 1) 
        {
            printf("Некорректный ввод. Попробуйте еще раз.\n");
            // очистка буфера ввода
            while (getchar() != '\n');
            continue;
        }

        if (line_num == 0) 
        {
            printf("Завершение работы.\n");
            break;
        }

        if (line_num < 1 || (size_t)line_num > line_count) 
        {
            printf("Ошибка: строки с номером %d не существует.\n", line_num);
            continue;
        }

        // индексация строк в интерфейсе с 1, в массиве с 0
        size_t idx = line_num - 1;
        off_t target_offset = table[idx].offset;
        size_t target_len = table[idx].length;

        // позиционируемся на начало заданной строки
        if (lseek(fd, target_offset, SEEK_SET) == (off_t)-1) 
        {
            perror("Ошибка позиционирования (lseek)");
            break;
        }

        // выделяем память под точную длину строки (+1 под ноль-терминатор)
        char *line_buf = malloc(target_len + 1);
        if (!line_buf) 
        {
            perror("Ошибка выделения памяти под строку");
            break;
        }

        // читаем точную длину строки
        ssize_t read_bytes = read(fd, line_buf, target_len);
        if (read_bytes == -1) 
        {
            perror("Ошибка чтения строки");
            free(line_buf);
            break;
        }

        line_buf[read_bytes] = '\0'; // добавляем ноль терминатор для корректной работы printf

        // выводим строку через printf()
        printf("Строка %d: %s", line_num, line_buf);
        if (read_bytes > 0 && line_buf[read_bytes - 1] != '\n') 
        {
            printf("\n"); // добавляем перевод строки если в файле его не было на конце
        }

        free(line_buf);
    }

    // освобождение ресурсов
    free(table);
    close(fd);

    return EXIT_SUCCESS;
}
