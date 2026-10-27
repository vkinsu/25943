#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_LINES 1000

int global_fd = -1;

struct LineInfo {
  off_t offset;
  size_t length;
};

void handle_alarm(int sig) {
  (void)sig;
  static const char msg[] =
      "\n[Таймаут!] Время на ввод истекло (5 секунд). Вывод всего файла:\n\n";
  write(STDOUT_FILENO, msg, sizeof(msg) - 1);

  if (global_fd >= 0) {
    char buffer[1024];
    ssize_t n;
    lseek(global_fd, 0L, SEEK_SET);
    while ((n = read(global_fd, buffer, sizeof(buffer))) > 0)
      write(STDOUT_FILENO, buffer, n);
    close(global_fd);
  }

  _exit(0);
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Использование: %s <имя_файла>\n", argv[0]);
    return 1;
  }

  int fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    perror("Ошибка открытия файла");
    return 1;
  }
  global_fd = fd;

  struct LineInfo table[MAX_LINES];
  int line_count = 0;
  table[0].offset = 0;
  table[0].length = 0;

  char ch;
  while (read(fd, &ch, 1) > 0) {
    table[line_count].length++;
    if (ch == '\n') {
      off_t current_pos = lseek(fd, 0L, SEEK_CUR);
      line_count++;
      if (line_count >= MAX_LINES) {
        fprintf(stderr,
                "Предупреждение: файл слишком большой, обрезано до %d строк.\n",
                MAX_LINES);
        break;
      }
      table[line_count].offset = current_pos;
      table[line_count].length = 0;
    }
  }

  if (line_count < MAX_LINES && table[line_count].length > 0)
    line_count++;

  printf("--- Таблица отступов и длин ---\n");
  printf("Строка\tОтступ\tДлина\n");
  for (int i = 0; i < line_count; i++)
    printf("%d\t%ld\t%lu\n", i + 1, (long)table[i].offset,
           (unsigned long)table[i].length);
  printf("-------------------------------\n");

  signal(SIGALRM, handle_alarm);

  int first = 1;
  int target_line;
  while (1) {
    if (first)
      printf("\nВведите номер строки (1-%d, или 0 для выхода) [У вас есть 5 "
             "секунд]: ",
             line_count);
    else
      printf("\nВведите номер строки (1-%d, или 0 для выхода): ", line_count);
    fflush(stdout);

    if (first)
      alarm(5);

    int rc = scanf("%d", &target_line);

    if (first) {
      alarm(0);
      first = 0;
    }

    if (rc == EOF)
      break;

    if (rc != 1) {
      int c;
      while ((c = getchar()) != '\n' && c != EOF)
        ;
      printf("Ошибка: введите число!\n");
      continue;
    }

    if (target_line == 0) {
      printf("Выход из программы.\n");
      break;
    }

    if (target_line < 1 || target_line > line_count) {
      printf("Ошибка: Неверный номер строки!\n");
      continue;
    }

    int idx = target_line - 1;
    char *line_buf = malloc(table[idx].length + 1);
    if (line_buf == NULL) {
      perror("Ошибка malloc");
      close(fd);
      return 1;
    }

    lseek(fd, table[idx].offset, SEEK_SET);
    ssize_t got = read(fd, line_buf, table[idx].length);
    if (got < 0)
      got = 0;
    line_buf[got] = '\0';

    printf("Строка %d: %s", target_line, line_buf);
    if (got == 0 || line_buf[got - 1] != '\n')
      printf("\n");

    free(line_buf);
  }

  close(fd);
  return 0;
}