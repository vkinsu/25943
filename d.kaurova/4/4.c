#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

struct node {
    char *str;
    struct node *next;
};

int main(void) {
    char buf[1024];
    struct node *head = NULL, *tail = NULL;

    while (1) {
        int c;
        size_t len = 0;

        while ((c = getchar()) != EOF && c != '\n') {

            if (c == 0x1B) {
                int next;
                while ((next = getchar()) != EOF) {
                    if ((next >= 'A' && next <= 'Z') ||
                        (next >= 'a' && next <= 'z') ||
                        next == '~') {
                        break;
                    }
                }
                continue;
            }

            if (!isprint((unsigned char)c) && c != '\t') {
                continue;
            }

            if (len + 1 < sizeof(buf)) {
                buf[len++] = (char)c;
            }
        }

        if (c == EOF && len == 0) break;

        buf[len++] = '\n';
        buf[len] = '\0';

        if (buf[0] == '.') break;

        size_t n = strlen(buf) + 1;
        char *copy = malloc(n);
        memcpy(copy, buf, n);

        struct node *p = malloc(sizeof(struct node));
        p->str = copy;
        p->next = NULL;

        if (head == NULL) {
            head = tail = p;
        } else {
            tail->next = p;
            tail = p;
        }
    }

    for (struct node *p = head; p; p = p->next)
        printf("%s", p->str);

    while (head) {
        struct node *next = head->next;
        free(head->str);
        free(head);
        head = next;
    }

    return 0;
}
