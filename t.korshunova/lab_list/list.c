#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXLINE 1024

struct node {
    char *str;
    struct node *next;
};

int main()
{
    char buf[MAXLINE];
    struct node *head = NULL;
    struct node *tail = NULL;
    struct node *p;
    struct node *tmp;

    printf("Enter strings (dot at start of line to finish):\n");

    while (1) {
        if (fgets(buf, MAXLINE, stdin) == NULL)
            break;

        if (buf[0] == '.')
            break;

        p = (struct node *) malloc(sizeof(struct node));
        if (p == NULL) {
            perror("malloc");
            return 1;
        }

        p->str = (char *) malloc(strlen(buf) + 1);
        if (p->str == NULL) {
            perror("malloc");
            return 1;
        }
        strcpy(p->str, buf);
        p->next = NULL;

        if (head == NULL) {
            head = p;
            tail = p;
        } else {
            tail->next = p;
            tail = p;
        }
    }

    printf("\n--- Result ---\n");
    p = head;
    while (p != NULL) {
        printf("%s", p->str);
        p = p->next;
    }

    p = head;
    while (p != NULL) {
        tmp = p;
        p = p->next;
        free(tmp->str);
        free(tmp);
    }

    return 0;
}


