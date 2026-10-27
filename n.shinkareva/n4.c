
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// один элемент списка хранит строку и адрес следующего элемента
struct StringNode {
    char *text;
    struct StringNode *next;
};

// читаем целую строку частями, даже если она длиннее буфера
// возвращаем 1 при успехе, 0 при конце ввода, -1 при ошибке
int read_line(char **result)
{
    char buffer[256];
    char *line = NULL;
    size_t length = 0;
    int has_input = 0;

    *result = NULL;

    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        size_t part_length = strlen(buffer);
        char *new_line;

        has_input = 1;

        // защищаем размер памяти от переполнения
        if (part_length > (size_t)-1 - length - 1) {
            free(line);
            return -1;
        }

        // увеличиваем память на размер прочитанной части
        new_line = realloc(line, length + part_length + 1);

        if (new_line == NULL) {
            free(line);
            return -1;
        }

        line = new_line;

        // добавляем новую часть к общей строке
        memcpy(line + length, buffer, part_length + 1);
        length += part_length;

        // если встретили конец строки, прекращаем чтение
        if (part_length > 0 && buffer[part_length - 1] == '\n')
            break;
    }

    // проверяем ошибку чтения
    if (ferror(stdin)) {
        free(line);
        return -1;
    }

    // если данные закончились, завершаем ввод
    if (!has_input)
        return 0;

    // убираем символ перевода строки
    if (length > 0 && line[length - 1] == '\n')
        line[--length] = '\0';

    // убираем возврат каретки при необходимости
    if (length > 0 && line[length - 1] == '\r')
        line[--length] = '\0';

    *result = line;
    return 1;
}

// безопасно выводим строку с управляющими символами
void print_safe(const char *text)
{
    const unsigned char *symbol = (const unsigned char *)text;

    while (*symbol != '\0') {

        // обычные символы и табуляцию выводим без изменений
        if (*symbol == '\t' ||
            (*symbol >= 32 && *symbol != 127)) {
            putchar(*symbol);
        }
        else {
            // управляющие символы показываем как текст
            printf("\\x%02X", *symbol);
        }

        symbol++;
    }

    putchar('\n');
}

// освобождаем память всех элементов списка
void free_list(struct StringNode *first)
{
    while (first != NULL) {
        struct StringNode *next = first->next;

        free(first->text);
        free(first);

        first = next;
    }
}

int main(void)
{
    struct StringNode *first = NULL;
    struct StringNode *last = NULL;
    struct StringNode *current;

    char *line;
    int status;

    printf("enter lines (dot at the beginning to stop):\n");

    while (1) {
        printf("> ");
        fflush(stdout);

        // считываем одну полную строку
        status = read_line(&line);

        // проверяем ошибки ввода или выделения памяти
        if (status == -1) {
            fprintf(stderr, "input or memory error\n");
            free_list(first);
            return 1;
        }

        // завершаем ввод при конце файла
        if (status == 0)
            break;

        // точка в начале строки завершает ввод
        if (line[0] == '.') {
            free(line);
            break;
        }

        // создаем новый элемент списка
        current = malloc(sizeof(*current));

        if (current == NULL) {
            perror("malloc");
            free(line);
            free_list(first);
            return 1;
        }

        current->text = line;
        current->next = NULL;

        // добавляем элемент в конец списка
        if (last == NULL)
            first = current;
        else
            last->next = current;

        last = current;
    }

    printf("\nentered lines:\n");

    // выводим все сохраненные строки
    for (current = first; current != NULL; current = current->next) {
        print_safe(current->text);
    }

    // освобождаем всю выделенную память
    free_list(first);

    return 0;
}