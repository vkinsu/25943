#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>


void print_uids(const char *stage) {
  printf("[%s]\n", stage);
  printf("  Реальный UID (RUID):    %ld\n", (long)getuid());
  printf("  Эффективный UID (EUID): %ld\n", (long)geteuid());
}

void try_open_file(const char *filename) {
  FILE *fp = fopen(filename, "r");
  if (fp == NULL) {
    perror("  fopen");
  } else {
    printf("  Файл '%s' успешно открыт!\n", filename);
    if (fclose(fp) != 0) {
      perror("  fclose");
    }
  }
}

int main(int argc, char *argv[]) {
  const char *filename = (argc > 1) ? argv[1] : "file.txt";

  printf("=== Шаг 1: До выравнивания UID ===\n");
  print_uids("Начальное состояние");
  try_open_file(filename);

  /*
   * Вызов setuid(getuid()) сбрасывает эффективный UID (EUID)
   * до реального UID (RUID) вызывающего пользователя.
   */
  if (setuid(getuid()) == -1) {
    perror("setuid");
    return 1;
  }

  printf("\n=== Шаг 2: После выравнивания UID ===\n");
  print_uids("После setuid(getuid())");
  try_open_file(filename);

  return 0;
}