#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 1024

typedef struct Node {
    char *text;
    struct Node *next;
} Node;

void remove_arrows(char *text){
    size_t read = 0;
    size_t write = 0;

    while (text[read] != '\0') {
        if (text[read] == '\x1b' && (text[read + 1] == '[' || text[read + 1] == 'O')) {
            size_t end = read + 2;

            while (text[end] >= 0x30 && text[end] <= 0x3f) {
                end++;
            }
            while (text[end] >= 0x20 && text[end] <= 0x2f) {
                end++;
            }
            if (text[end] >= 'A' && text[end] <= 'D') {
                read = end + 1;
                continue;
            }
        }
        text[write++] = text[read++];
    }
    text[write] = '\0';
}

void free_list(Node *head){
    while (head != NULL) {
        Node *next = head->next;
        free(head->text);
        free(head);
        head = next;
    }
}

int main(void){
    char buffer[BUFFER_SIZE];
    Node *head = NULL;
    Node *tail = NULL;

    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        remove_arrows(buffer);
        if (buffer[0] == '.') {
            break;
        }
        size_t length = strlen(buffer);
        if (length > 0 && buffer[length - 1] == '\n') {
            buffer[--length] = '\0';
        }
        Node *node = malloc(sizeof(*node));
        if (node == NULL) {
            fprintf(stderr, "Ошибка выделения памяти\n");
            free_list(head);
            return EXIT_FAILURE;
        }
        node->text = malloc(length + 1);
        if (node->text == NULL) {
            fprintf(stderr, "Ошибка выделения памяти\n");
            free(node);
            free_list(head);
            return EXIT_FAILURE;
        }
        strcpy(node->text, buffer);
        node->next = NULL;
        if (tail != NULL) {
            tail->next = node;
        } else {
            head = node;
        }
        tail = node;
    }
    for (Node *current = head; current != NULL;
         current = current->next) {
        puts(current->text);
    }
    free_list(head);
    return EXIT_SUCCESS;
}