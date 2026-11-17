#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

typedef struct StringNode
{
    char *text;
    struct StringNode *next;
} StringNode;

void release_strings(StringNode *first)
{
    while (first != NULL)
    {
        StringNode *old = first;
        first = first->next;
        free(old->text);
        free(old);
    }
}

void show_strings(StringNode *first)
{
    StringNode *current = first;
    while (current != NULL)
    {
        printf("%s", current->text);
        current = current->next;
    }
}

void add_string(StringNode **first, StringNode **last, const char *text)
{
    size_t size = strlen(text);

    char *copy = (char *)malloc(size + 1);
    if (copy == NULL)
    {
        fprintf(stderr, "Malloc for string Error!\n");
        exit(1);
    }
    strcpy(copy, text);

    StringNode *element = (StringNode *)malloc(sizeof(StringNode));
    if (element == NULL)
    {
        fprintf(stderr, "Malloc for Node error!\n");
        exit(1);
    }
    element->text = copy;
    element->next = NULL;

    if (*first == NULL)
    {
        *first = element;
        *last = element;
    }
    else
    {
        (*last)->next = element;
        *last = element;
    }
}

int main(void)
{
    const int LINE_SIZE = 1024;
    char line[LINE_SIZE];

    StringNode *first = NULL;
    StringNode *last = NULL;

    printf("Write string ('.' at start ends input): ");

    while(1)
    {

        if (fgets(line, LINE_SIZE, stdin) == NULL)
            break;

        if (line[0] == '.')
            break;

        printf("Write string ('.' at start ends input): ");
        add_string(&first, &last, line);
    }

    printf("\nList contains:\n");
    show_strings(first);

    release_strings(first);

    return 0;
}
