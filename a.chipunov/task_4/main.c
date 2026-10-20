#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAXLINE 1024

struct node {
    char *str;
    struct node *next;
};

static void clean(char *s)
{
    char *d = s;

    while (*s) {
        if (*s == '\033') {
            s++;
            if (*s == '[' || *s == 'O') {
                s++;
                while (*s && (*s < 0x40 || *s > 0x7e))
                    s++;
                if (*s)
                    s++;
            } else if (*s) {
                s++;
            }
        } else if (iscntrl((unsigned char)*s) && *s != '\n' && *s != '\t') {
            s++;
        } else {
            *d++ = *s++;
        }
    }
    *d = '\0';
}

int main(void)
{
    char buf[MAXLINE];
    struct node *head = NULL, *tail = NULL, *p, *tmp;
    size_t len;

    while (fgets(buf, sizeof(buf), stdin) != NULL) {
        clean(buf);
        if (buf[0] == '.')
            break;

        len = strlen(buf);

        p = malloc(sizeof(struct node));
        if (p == NULL) {
            perror("malloc");
            return 1;
        }
        p->str = malloc(len + 1);
        if (p->str == NULL) {
            perror("malloc");
            return 1;
        }
        strcpy(p->str, buf);
        p->next = NULL;

        /* append to end */
        if (head == NULL)
            head = p;
        else
            tail->next = p;
        tail = p;
    }

    for (p = head; p != NULL; p = p->next)
        fputs(p->str, stdout);

    for (p = head; p != NULL; p = tmp) {
        tmp = p->next;
        free(p->str);
        free(p);
    }
    return 0;
}

