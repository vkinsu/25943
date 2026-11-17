#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Node {
    char *str;
    struct Node *next;
} Node;

void clean_input(char *str) {
    int r = 0, w = 0;
    while (str[r] != '\0') {
        if (str[r] == '\033') {
            r++;
            if (str[r] == '[') {
                r++;
                while (str[r] != '\0' && str[r] >= '0' && str[r] <= '?') {
                    r++;
                }
                if (str[r] != '\0' && str[r] >= '@' && str[r] <= '~') {
                    r++;
                }
            }
        } else {
            str[w++] = str[r++];
        }
    }
    str[w] = '\0';
}

int main(void) {
    char buffer[4096];
    Node *head = NULL;
    Node *tail = NULL;

    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        clean_input(buffer);
        
        if (buffer[0] == '.') {
            break;
        }

        size_t len = strlen(buffer);
        if (len == 0) continue;

        char *new_str = (char *)malloc(len + 1);
        if (new_str == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        strcpy(new_str, buffer);

        Node *new_node = (Node *)malloc(sizeof(Node));
        if (new_node == NULL) {
            perror("malloc");
            free(new_str);
            exit(EXIT_FAILURE);
        }

        new_node->str = new_str;
        new_node->next = NULL;

        if (head == NULL) {
            head = new_node;
            tail = new_node;
        } else {
            tail->next = new_node;
            tail = new_node;
        }
    }

    Node *current = head;
    while (current != NULL) {
        printf("%s", current->str);
        current = current->next;
    }

    current = head;
    while (current != NULL) {
        Node *temp = current;
        current = current->next;
        free(temp->str);
        free(temp);
    }

    return 0;
}
