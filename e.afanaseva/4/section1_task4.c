
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_LINE 1024

struct Node
{
    char *str;
    struct Node *next;
};

void remove_arrows(char *str)
{
    char *read_ptr = str;
    char *write_ptr = str;

    while (*read_ptr != '\0')
    {
        if ((unsigned char)read_ptr[0] == 27 &&
            read_ptr[1] == '[' &&
            read_ptr[2] != '\0' &&
            (read_ptr[2] == 'A' ||
             read_ptr[2] == 'B' ||
             read_ptr[2] == 'C' ||
             read_ptr[2] == 'D'))
        {
            read_ptr += 3;
        }
        else
        {
            *write_ptr++ = *read_ptr++;
        }
    }

    *write_ptr = '\0';
}

int main(void)
{
    char buffer[MAX_LINE];
    struct Node *head = NULL;
    struct Node *tail = NULL;
    int error = 0;

    while (1)
    {
        char *line = NULL;
        size_t length = 0;

        printf("Введите строку: ");
        fflush(stdout);

        while (fgets(buffer, sizeof(buffer), stdin) != NULL)
        {
            size_t part_len = strlen(buffer);

            if (part_len > SIZE_MAX - length - 1)
            {
                fprintf(stderr, "Слишком длинная строка\n");
                free(line);
                error = 1;
                line = NULL;
                break;
            }

            char *new_line = realloc(line, length + part_len + 1);

            if (new_line == NULL)
            {
                perror("realloc");
                free(line);
                error = 1;
                line = NULL;
                break;
            }

            line = new_line;
            memcpy(line + length, buffer, part_len + 1);
            length += part_len;

            if (length > 0 && line[length - 1] == '\n')
            {
                break;
            }
        }

        if (error)
        {
            break;
        }

        if (line == NULL)
        {
            break;
        }

        size_t original_length = strlen(line);

        remove_arrows(line);

        if (line[0] == '.')
        {
            free(line);
            break;
        }

        if (original_length > strlen(line) &&
            (line[0] == '\n' || line[0] == '\0'))
        {
            free(line);
            continue;
        }

        struct Node *new_node = malloc(sizeof(struct Node));

        if (new_node == NULL)
        {
            perror("malloc");
            free(line);
            error = 1;
            break;
        }

        new_node->str = line;
        new_node->next = NULL;

        if (head == NULL)
        {
            head = new_node;
            tail = new_node;
        }
        else
        {
            tail->next = new_node;
            tail = new_node;
        }
    }

    printf("\nСписок строк:\n");

    if (head == NULL)
    {
        printf("Список пуст\n");
    }
    else
    {
        struct Node *current = head;

        while (current != NULL)
        {
            printf("%s", current->str);
            current = current->next;
        }
    }

    struct Node *current = head;

    while (current != NULL)
    {
        struct Node *next = current->next;

        free(current->str);
        free(current);

        current = next;
    }

    return error ? 1 : 0;
}