
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>

// информация о строке: ее начало и длина в байтах
typedef struct {
    size_t offset;
    size_t length;
} LineInfo;

// флаг окончания времени
volatile sig_atomic_t time_is_up = 0;

// обработчик сигнала от таймера
void alarm_handler(int signal_number)
{
    (void)signal_number;
    time_is_up = 1;
}

// считываем строку без переполнения буфера
// 1 - успех, 0 - конец ввода, -1 - ошибка
// -2 - время вышло, -3 - слишком длинный ввод
int input_line(char *text, size_t size, int use_timer)
{
    size_t length = 0;
    int too_long = 0;
    int symbol;

    if (use_timer) {
        time_is_up = 0;
        alarm(5);
    }

    while (1) {
        errno = 0;
        symbol = getchar();

        // проверяем окончание времени
        if (use_timer && time_is_up) {
            alarm(0);
            clearerr(stdin);
            return -2;
        }

        if (symbol == EOF) {
            if (use_timer)
                alarm(0);

            if (ferror(stdin))
                return -1;

            if (length == 0 && !too_long)
                return 0;

            break;
        }

        if (symbol == '\n')
            break;

        // записываем символы, пока хватает места
        if (length + 1 < size)
            text[length++] = (char)symbol;
        else
            too_long = 1;
    }

    if (use_timer) {
        alarm(0);

        if (time_is_up)
            return -2;
    }

    text[length] = '\0';

    return too_long ? -3 : 1;
}

// добавляем строку в таблицу
int add_line(LineInfo **table, size_t *count,
             size_t *capacity, size_t start, size_t end)
{
    if (*count == *capacity) {
        size_t new_capacity =
            (*capacity == 0) ? 16 : *capacity * 2;

        LineInfo *new_table;

        // проверяем допустимый размер массива
        if (new_capacity < *capacity ||
            new_capacity > SIZE_MAX / sizeof(LineInfo))
            return -1;

        // увеличиваем память для таблицы
        new_table = realloc(
            *table, new_capacity * sizeof(LineInfo)
        );

        if (new_table == NULL)
            return -1;

        *table = new_table;
        *capacity = new_capacity;
    }

    (*table)[*count].offset = start;
    (*table)[*count].length = end - start;

    (*count)++;
    return 0;
}

int main(void)
{
    char filename[256];
    char input[128];

    int file;
    struct stat file_info;
    size_t file_size;
    char *data;

    LineInfo *table = NULL;
    size_t count = 0;
    size_t capacity = 0;
    size_t start = 0;
    size_t i;

    struct sigaction action;

    int first_input = 1;
    int result = 0;
    int status;

    long line_number;
    char *end;

    // запрашиваем имя текстового файла
    printf("введите имя текстового файла: ");
    fflush(stdout);

    status = input_line(filename, sizeof(filename), 0);

    if (status != 1 || filename[0] == '\0') {
        printf("неверное имя файла\n");
        return 1;
    }

    // открываем файл только для чтения
    file = open(filename, O_RDONLY);

    if (file == -1) {
        perror("open");
        return 1;
    }

    // узнаем размер файла
    if (fstat(file, &file_info) == -1) {
        perror("fstat");
        close(file);
        return 1;
    }

    // проверяем, что это обычный файл
    if (!S_ISREG(file_info.st_mode)) {
        printf("нужен обычный текстовый файл\n");
        close(file);
        return 1;
    }

    if (file_info.st_size == 0) {
        printf("файл пуст\n");
        close(file);
        return 0;
    }

    if (file_info.st_size < 0 ||
        (uintmax_t)file_info.st_size > SIZE_MAX) {
        printf("слишком большой файл\n");
        close(file);
        return 1;
    }

    file_size = (size_t)file_info.st_size;

    // отображаем содержимое файла в память
    data = mmap(NULL, file_size, PROT_READ,
                MAP_PRIVATE, file, 0);

    if (data == MAP_FAILED) {
        perror("mmap");
        close(file);
        return 1;
    }

    // после mmap файловый дескриптор больше не нужен
    close(file);

    // строим таблицу строк по символам перевода строки
    for (i = 0; i < file_size; i++) {
        if (data[i] == '\n') {
            if (add_line(&table, &count, &capacity,
                         start, i + 1) == -1) {
                fprintf(stderr, "ошибка выделения памяти\n");
                result = 1;
                goto cleanup;
            }

            start = i + 1;
        }
    }

    // учитываем последнюю строку без символа перевода строки
    if (start < file_size &&
        add_line(&table, &count, &capacity,
                 start, file_size) == -1) {
        fprintf(stderr, "ошибка выделения памяти\n");
        result = 1;
        goto cleanup;
    }

    // выводим таблицу смещений и длин строк
    printf("\nномер  смещение  длина\n");

    for (i = 0; i < count; i++) {
        printf("%-6zu %-9zu %zu\n",
               i + 1,
               table[i].offset,
               table[i].length);
    }

    // настраиваем обработчик сигнала SIGALRM
    memset(&action, 0, sizeof(action));
    action.sa_handler = alarm_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if (sigaction(SIGALRM, &action, NULL) == -1) {
        perror("sigaction");
        result = 1;
        goto cleanup;
    }

    // запрашиваем номера строк
    while (1) {
        printf("\nномер строки (0 - выход)%s: ",
               first_input ? " [5 секунд]" : "");

        fflush(stdout);

        // ограничиваем время только первого ввода
        status = input_line(input, sizeof(input), first_input);
        first_input = 0;

        // если время вышло, печатаем весь файл
        if (status == -2) {
            printf("\nвремя вышло! содержимое файла:\n");

            if (fwrite(data, 1, file_size, stdout) != file_size) {
                perror("fwrite");
                result = 1;
            }

            if (data[file_size - 1] != '\n')
                putchar('\n');

            break;
        }

        // конец ввода
        if (status == 0)
            break;

        if (status == -1) {
            perror("ввод");
            result = 1;
            break;
        }

        if (status == -3) {
            printf("слишком длинный ввод\n");
            continue;
        }

        // преобразуем введенный номер в число
        errno = 0;
        line_number = strtol(input, &end, 10);

        // пропускаем пробелы после числа
        while (isspace((unsigned char)*end))
            end++;

        // проверяем правильность введенного номера
        if (end == input || *end != '\0' ||
            errno == ERANGE || line_number < 0) {
            printf("неверный номер строки\n");
            continue;
        }

        // ноль завершает работу программы
        if (line_number == 0)
            break;

        // проверяем существование строки
        if ((uintmax_t)line_number > count) {
            printf("такой строки нет\n");
            continue;
        }

        // получаем строку из отображенной памяти
        LineInfo selected = table[(size_t)line_number - 1];

        printf("строка %ld: ", line_number);

        // выводим выбранную строку
        if (fwrite(data + selected.offset, 1,
                   selected.length, stdout) != selected.length) {
            perror("fwrite");
            result = 1;
            break;
        }

        // добавляем перенос, если его не было в файле
        if (data[selected.offset + selected.length - 1] != '\n')
            putchar('\n');
    }

cleanup:
    // освобождаем память таблицы
    free(table);

    // удаляем отображение файла из памяти
    if (munmap(data, file_size) == -1) {
        perror("munmap");
        result = 1;
    }

    return result;
}
