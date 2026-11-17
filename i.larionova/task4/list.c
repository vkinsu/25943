#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXLINE 1024

struct node {
    char *str;           // указатель на строку (в куче)
    struct node *next;   // указатель на следующий узел
};

int main(void)
{
    // буфер для строк
    char buf[MAXLINE];

    struct node *head = NULL;
    struct node *tail = NULL;

    // читаем строки
    while (fgets(buf, sizeof(buf), stdin) != NULL) 
    {
        // если начинается с точки - конец ввода
        if (buf[0] == '.')
            break;

        size_t len = strlen(buf);

        // убрать \n, если он есть в буфере
        if (len > 0 && buf[len - 1] == '\n') 
        {
            buf[len - 1] = '\0';
            len--; // обновляем длину без учета '\n'
        } 
        else if (len == MAXLINE - 1) 
        {
            // если строка длиннее MAXLINE, вычищаем остаток из stdin
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
        }

        // выделить память под строку
        char *copy = (char *)malloc(len + 1);
        if (copy == NULL) 
        { 
            perror("malloc"); 
            exit(1); 
        }
        memcpy(copy, buf, len + 1);

        // выделить память под узел
        struct node *n = (struct node *)malloc(sizeof(*n));
        if (n == NULL) 
        { 
            perror("malloc"); 
            exit(1); 
        }
        n->str = copy;
        n->next = NULL;

        // прицепить в конец
        if (head == NULL) 
        {
            head = n;
            tail = n;
        } 
        else 
        {
            tail->next = n;
            tail = n;
        }
    }

    // печатаем
    for (struct node *p = head; p != NULL; p = p->next)
        printf("%s\n", p->str);

    // освобождаем
    struct node *p = head;
    while (p != NULL) 
    {
        struct node *next = p->next;
        free(p->str);
        free(p);
        p = next;
    }

    return 0;
}