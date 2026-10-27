#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_LINES 1000

struct LineInfo {
  off_t offset;
  size_t length;
};

static volatile sig_atomic_t timed_out = 0;

static void handle_alarm(int sig) {
  (void)sig;
  timed_out = 1;
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

  struct stat st;
  if (fstat(fd, &st) == -1) {
    perror("Ошибка fstat");
    close(fd);
    return 1;
  }

  if (st.st_size == 0) {
    printf("Файл пуст.\n");
    close(fd);
    return 0;
  }

  size_t size = (size_t)st.st_size;
  char *map = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
  if (map == MAP_FAILED) {
    perror("Ошибка mmap");
    close(fd);
    return 1;
  }
  close(fd);

  struct LineInfo table[MAX_LINES];
  int line_count = 0;
  table[0].offset = 0;
  table[0].length = 0;

  for (size_t pos = 0; pos < size; ++pos) {
    table[line_count].length++;
    if (map[pos] == '\n') {
      line_count++;
      if (line_count >= MAX_LINES) {
        fprintf(stderr,
                "Предупреждение: файл слишком большой, обрезано до %d строк.\n",
                MAX_LINES);
        break;
      }
      table[line_count].offset = (off_t)(pos + 1);
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

  struct sigaction sa;
  sa.sa_handler = handle_alarm;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGALRM, &sa, NULL);

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

    if (timed_out) {
      printf("\n[Таймаут!] Время на ввод истекло (5 секунд). Вывод всего "
             "файла:\n\n");
      printf("%.*s", (int)size, map);
      munmap(map, size);
      return 0;
    }

    if (rc == EOF)
      break;

    if (rc != 1) {
      int c;
      while ((c = getchar()) != '\n' && c != EOF)
        ;
      printf("Ошибка: Введите число!\n");
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
    printf("Строка %d: %.*s", target_line, (int)table[idx].length,
           map + table[idx].offset);
  }

  munmap(map, size);
  return 0;
}