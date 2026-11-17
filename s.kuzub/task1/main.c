#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

extern char **environ;

struct Option {
  int type;
  char *argument;
};

int main(int argc, char *argv[]) {
  /* Запас памяти под случай слитных опций (например, ./main -ispu) */
  int max_opts = 1;
  for (int k = 1; k < argc; ++k) {
    max_opts += strlen(argv[k]);
  }

  struct Option *array = malloc(max_opts * sizeof(struct Option));
  if (array == NULL) {
    perror("Ошибка выделения памяти");
    return 1;
  }

  int opt;
  int opt_count = 0;

  /* Считываем опции слева направо */
  while ((opt = getopt(argc, argv, "ispuU:cC:dvV:")) != -1) {
    array[opt_count].type = opt;
    array[opt_count].argument = optarg;
    opt_count++;
  }

  /* Обработка опций в обратном порядке (справа налево) */
  for (int i = opt_count - 1; i >= 0; --i) {
    opt = array[i].type;
    char *arg = array[i].argument;

    switch (opt) {
    case 'i': {
      printf("UID: %ld, EUID: %ld, GID: %ld, EGID: %ld\n", (long)getuid(),
             (long)geteuid(), (long)getgid(), (long)getegid());
      break;
    }

    case 's': {
      if (setpgid(0, 0) == -1) {
        perror("Ошибка в setpgid");
      } else {
        printf("Процесс стал лидером группы. PGID: %ld\n", (long)getpgrp());
      }
      break;
    }

    case 'p': {
      printf("PID: %ld, PPID: %ld, PGID: %ld\n", (long)getpid(),
             (long)getppid(), (long)getpgrp());
      break;
    }

    case 'u': {
      long n = sysconf(_SC_CHILD_MAX);
      if (n == -1)
        printf("Макс. процессов: unlimited\n");
      else
        printf("Макс. процессов: %ld\n", n);
      break;
    }

    case 'U': {
      char *end;
      errno = 0;
      long val = strtol(arg, &end, 10);
      if (errno != 0 || *end != '\0' || val < 0) {
        fprintf(stderr, "Некорректное значение ulimit: %s\n", arg);
        break;
      }
      if (ulimit(UL_SETFSIZE, val) == -1) {
        perror("Ошибка в ulimit(UL_SETFSIZE)");
      } else {
        printf("Новое значение ulimit установлено: %ld\n", val);
      }
      break;
    }

    case 'c': {
      struct rlimit rl;
      if (getrlimit(RLIMIT_CORE, &rl) == -1) {
        perror("Ошибка в getrlimit(RLIMIT_CORE)");
      } else {
        if (rl.rlim_cur == RLIM_INFINITY)
          printf("Core size: unlimited\n");
        else
          printf("Core size: %llu байт\n", (unsigned long long)rl.rlim_cur);
      }
      break;
    }

    case 'C': {
      struct rlimit rl;
      if (getrlimit(RLIMIT_CORE, &rl) == -1) {
        perror("Ошибка в getrlimit(RLIMIT_CORE)");
      } else {
        rl.rlim_cur = (rlim_t)atol(arg);
        if (setrlimit(RLIMIT_CORE, &rl) == -1) {
          perror("Ошибка в setrlimit(RLIMIT_CORE)");
        } else {
          printf("Новый лимит core-файла установлен: %llu байт\n",
                 (unsigned long long)rl.rlim_cur);
        }
      }
      break;
    }

    case 'd': {
      char cwd[PATH_MAX];
      if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Текущая директория: %s\n", cwd);
      } else {
        perror("Ошибка в getcwd");
      }
      break;
    }

    case 'v': {
      printf("Переменные окружения:\n");
      for (char **env = environ; *env != NULL; ++env) {
        printf("  %s\n", *env);
      }
      break;
    }

    case 'V': {
      if (putenv(arg) != 0) {
        perror("Ошибка в putenv");
      } else {
        printf("Переменная окружения обновлена: %s\n", arg);
      }
      break;
    }

    case '?':
    default:
      fprintf(stderr, "Встречена некорректная опция или пропущен аргумент.\n");
      break;
    }
  }

  free(array);
  return 0;
}