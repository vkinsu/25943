#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 1024
#define TAB_WIDTH 8

struct Node {
  char *str;
  struct Node *next;
};

static char *read_line(void) {
  size_t cap = BUFFER_SIZE, len = 0;
  char *line = malloc(cap);
  if (line == NULL)
    return NULL;

  while (fgets(line + len, (int)(cap - len), stdin) != NULL) {
    len += strlen(line + len);
    if (len > 0 && line[len - 1] == '\n')
      return line;
    if (len + 1 == cap) {
      char *tmp = realloc(line, cap * 2);
      if (tmp == NULL) {
        free(line);
        return NULL;
      }
      line = tmp;
      cap *= 2;
    }
  }

  if (len > 0)
    return line;
  free(line);
  return NULL;
}

static char *sanitize(const char *src) {
  size_t n = strlen(src);
  char *dst = malloc(n * TAB_WIDTH + 1);
  if (dst == NULL)
    return NULL;

  size_t j = 0, col = 0;
  for (size_t i = 0; src[i] != '\0'; ++i) {
    unsigned char c = (unsigned char)src[i];

    if (c == 0x1b) {
      if (src[i + 1] == '[' || src[i + 1] == 'O') {
        i += 2;
        while (src[i] != '\0' && !((unsigned char)src[i] >= 0x40 &&
                                   (unsigned char)src[i] <= 0x7e))
          ++i;
        if (src[i] == '\0')
          break;
      }
      continue;
    }

    if (c == '\t') {
      do {
        dst[j++] = ' ';
        ++col;
      } while (col % TAB_WIDTH != 0);
      continue;
    }

    if (c == '\n') {
      dst[j++] = '\n';
      col = 0;
      continue;
    }

    if (c < 0x20 || c == 0x7f)
      continue;

    dst[j++] = (char)c;
    if ((c & 0xC0) != 0x80)
      ++col;
  }

  dst[j] = '\0';
  return dst;
}

static void free_list(struct Node *head) {
  while (head != NULL) {
    struct Node *tmp = head;
    head = head->next;
    free(tmp->str);
    free(tmp);
  }
}

int main(void) {
  struct Node *head = NULL;
  struct Node *tail = NULL;

  printf("Вводите строки (для завершения введите '.' в начале строки):\n");

  while (1) {
    char *raw = read_line();
    if (raw == NULL)
      break;

    char *clean = sanitize(raw);
    free(raw);
    if (clean == NULL) {
      fprintf(stderr, "Ошибка выделения памяти под строку!\n");
      free_list(head);
      return 1;
    }

    if (clean[0] == '.') {
      free(clean);
      break;
    }

    struct Node *new_node = malloc(sizeof(struct Node));
    if (new_node == NULL) {
      fprintf(stderr, "Ошибка выделения памяти под узел!\n");
      free(clean);
      free_list(head);
      return 1;
    }

    new_node->str = clean;
    new_node->next = NULL;

    if (head == NULL)
      head = new_node;
    else
      tail->next = new_node;
    tail = new_node;
  }

  printf("\nВведенные строки из списка:\n");
  for (struct Node *cur = head; cur != NULL; cur = cur->next)
    printf("%s", cur->str);

  free_list(head);
  return 0;
}