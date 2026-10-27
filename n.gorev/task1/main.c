#define __EXTENSIONS__ 1
#define _XOPEN_SOURCE 600

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/resource.h>

#define MAX_OPTIONS 100

extern char **environ;

/* строка -> неотрицательное число; -1 если это не число */
int parse_number(const char *text, long *result) {
    char *end;

    errno = 0;
    long value = strtol(text, &end, 10);

    if (errno == ERANGE || end == text || *end != '\0' || value < 0)
        return -1;

    *result = value;
    return 0;
}

void print_ids(void) {
    pid_t pid = getpid();
    pid_t pgrp = getpgrp();

    printf("PID:  %ld\n", (long)pid);
    printf("PPID: %ld\n", (long)getppid());
    printf("PGRP: %ld\n", (long)pgrp);
    printf("лидер группы: %s\n", pid == pgrp ? "да" : "нет");
}

int print_max_processes(void) {
#ifdef RLIMIT_NPROC
    struct rlimit limit;

    if (getrlimit(RLIMIT_NPROC, &limit) == -1) {
        perror("ошибка getrlimit");
        return -1;
    }

    if (limit.rlim_cur == RLIM_INFINITY)
        printf("max user processes (-u) unlimited\n");
    else
        printf("max user processes (-u) %llu\n", (unsigned long long)limit.rlim_cur);
#else
    errno = 0;
    long value = sysconf(_SC_CHILD_MAX);

    if (value == -1) {
        if (errno != 0) {
            perror("ошибка sysconf");
            return -1;
        }
        printf("max user processes (-u) unlimited\n");
    } else {
        printf("max user processes (-u) %ld\n", value);
    }
#endif
    return 0;
}

int set_max_processes(long value) {
#ifdef RLIMIT_NPROC
    struct rlimit limit;

    if (getrlimit(RLIMIT_NPROC, &limit) == -1) {
        perror("ошибка getrlimit");
        return -1;
    }

    limit.rlim_cur = (rlim_t)value;

    if (setrlimit(RLIMIT_NPROC, &limit) == -1) {
        printf("ulimit %ld: изменить нельзя\n", value);
        return -1;
    }
    return 0;
#else
    printf("ulimit %ld: изменить нельзя\n", value);
    return -1;
#endif
}

int main(int argc, char *argv[]) {
    char options[] = ":ispuU:cC:dvV:";
    int saved_options[MAX_OPTIONS];
    char *saved_arguments[MAX_OPTIONS];
    int count = 0;

    long value;
    char directory[1024];
    struct rlimit limit;

    opterr = 0;

    if (argc == 1) {
        printf("опции:\n");
        printf("-i            вывести UID и GID\n");
        printf("-s            стать лидером группы процессов\n");
        printf("-p            вывести PID, PPID и PGRP\n");
        printf("-u            вывести лимит процессов\n");
        printf("-Uvalue       установить лимит процессов\n");
        printf("-c            вывести лимит core-файла\n");
        printf("-Csize        установить лимит core-файла в байтах\n");
        printf("-d            вывести текущую директорию\n");
        printf("-v            вывести переменные окружения\n");
        printf("-Vname=value  установить переменную окружения\n");
        return 0;
    }

    /* сначала собираем все опции */
    int option;
    while ((option = getopt(argc, argv, options)) != -1) {
        if (option == '?') {
            printf("неизвестная опция %c\n", optopt);
            return 1;
        }

        if (option == ':') {
            printf("опции -%c нужен аргумент\n", optopt);
            return 1;
        }

        if (count >= MAX_OPTIONS) {
            printf("слишком много опций\n");
            return 1;
        }

        saved_options[count] = option;
        saved_arguments[count] = optarg;
        count++;
    }

    if (optind < argc) {
        printf("лишний аргумент: %s\n", argv[optind]);
        return 1;
    }

    /* выполняем опции в обратном порядке */
    for (int i = count - 1; i >= 0; i--) {
        switch (saved_options[i]) {
        case 'i':
            printf("ruid=%ld euid=%ld\n", (long)getuid(), (long)geteuid());
            printf("rgid=%ld egid=%ld\n", (long)getgid(), (long)getegid());
            break;

        case 's':
            printf("до:\n");
            print_ids();

            if (setpgid(0, 0) == -1 && getpid() != getpgrp())
                perror("ошибка setpgid");

            printf("после:\n");
            print_ids();
            break;

        case 'p':
            print_ids();
            break;

        case 'u':
            print_max_processes();
            break;

        case 'U':
            if (parse_number(saved_arguments[i], &value) == -1) {
                printf("плохое значение для U: %s\n", saved_arguments[i]);
                break;
            }
            set_max_processes(value);
            break;

        case 'c':
            if (getrlimit(RLIMIT_CORE, &limit) == -1)
                perror("ошибка getrlimit");
            else if (limit.rlim_cur == RLIM_INFINITY)
                printf("core=unlimited\n");
            else
                printf("core=%llu bytes\n", (unsigned long long)limit.rlim_cur);
            break;

        case 'C':
            if (parse_number(saved_arguments[i], &value) == -1) {
                printf("плохое значение для C: %s\n", saved_arguments[i]);
                break;
            }

            if (getrlimit(RLIMIT_CORE, &limit) == -1) {
                perror("ошибка getrlimit");
                break;
            }

            limit.rlim_cur = (rlim_t)value;

            if (setrlimit(RLIMIT_CORE, &limit) == -1)
                perror("ошибка setrlimit");
            break;

        case 'd':
            if (getcwd(directory, sizeof(directory)) == NULL)
                perror("ошибка getcwd");
            else
                printf("%s\n", directory);
            break;

        case 'v':
            for (int j = 0; environ[j] != NULL; j++)
                printf("%s\n", environ[j]);
            break;

        case 'V':
            if (saved_arguments[i][0] == '=' || strchr(saved_arguments[i], '=') == NULL)
                printf("плохое значение для V: %s\n", saved_arguments[i]);
            else if (putenv(saved_arguments[i]) != 0)
                perror("ошибка putenv");
            break;
        }
    }

    return 0;
}