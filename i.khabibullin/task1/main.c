#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/resource.h>
#include <errno.h>
/*
-i  Печатает реальные и эффективные идентификаторы пользователя и группы.
-s  Процесс становится лидером группы. Подсказка: смотри setpgid(2).
-p  Печатает идентификаторы процесса, процесса-родителя и группы процессов.
-u  Печатает значение ulimit
-Unew_ulimit  Изменяет значение ulimit. Подсказка: смотри atol(3C) на странице руководства strtol(3C)
-c  Печатает размер в байтах core-файла, который может быть создан.
-Csize  Изменяет размер core-файла
-d  Печатает текущую рабочую директорию
-v  Распечатывает переменные среды и их значения
-Vname=value  Вносит новую переменную в среду или изменяет значение существующей переменной.
*/

extern char **environ;
// -i
void printfl_i()
{
    uid_t r_uid = getuid();
    uid_t e_uid = geteuid();

    gid_t r_gid = getgid();
    gid_t e_gid = getegid();

    printf("RUID: %d, EUID: %d\n", r_uid, e_uid);
    printf("RGID: %d, EGID: %d\n", r_gid, e_gid);

    printf("--------------------------------------\n");
}

// -p
void printfl_p()
{
    pid_t pid = getpid();
    pid_t ppid = getppid();
    pid_t pgrp = getpgrp();

    printf("PID: %d, PPID: %d, PGID: %d\n", pid, ppid, pgrp);
    printf("--------------------------------------\n");
}

// -u
void printfl_u()
{
    struct rlimit lim;
    if (getrlimit(RLIMIT_NOFILE, &lim) == -1)
    {
        fprintf(stderr, "ulimit reading error: %s\n", strerror(errno));
        return;
    }

    printf("Soft limit: %ld, hard limit: %ld\n", lim.rlim_cur, lim.rlim_max);
    printf("--------------------------------------\n");
}

// -U
void printfl_U(long new_soft_ulimit)
{
    struct rlimit lim;

    if (getrlimit(RLIMIT_NOFILE, &lim) == -1)
    {
        fprintf(stderr, "ulimit reading error: %s\n", strerror(errno));
        return;
    }

    if ((rlim_t)new_soft_ulimit > lim.rlim_max)
    {
        fprintf(stderr, "Error: new ulimit %ld is greater than hard limit %ld\n", (long)new_soft_ulimit, (long)lim.rlim_max);
        return;
    }

    lim.rlim_cur = new_soft_ulimit;

    if (setrlimit(RLIMIT_NOFILE, &lim) == -1)
    {
        fprintf(stderr, "Error changing ulimit: %s\n", strerror(errno));
        return;
    }
}

// -c
void printfl_c()
{
    struct rlimit lim;

    if (getrlimit(RLIMIT_CORE, &lim) == -1)
    {
        fprintf(stderr, "Error getting size of core file: %s\n", strerror(errno));
        return;
    }

    if (lim.rlim_cur == RLIM_INFINITY)
    {
        printf("Current limit of core-file: unlimited\n");
    }
    else
    {
        printf("Current limit core-file: %lu bytes\n", (unsigned long)lim.rlim_cur);
    }

    if (lim.rlim_max == RLIM_INFINITY)
    {
        printf("Hard core-file limit: unlimited\n");
    }
    else
    {
        printf("Hard core-file limit: %lu bytes\n", (unsigned long)lim.rlim_max);
    }
    printf("--------------------------------------\n");
}

// -C
void printfl_C(unsigned long bytes)
{
    struct rlimit lim;

    if (getrlimit(RLIMIT_CORE, &lim) == -1)
    {
        fprintf(stderr, "Error reading limits of core-file: %s\n", strerror(errno));
        return;
    }

    if (lim.rlim_max != RLIM_INFINITY && bytes > lim.rlim_max)
    {
        fprintf(stderr, "Error: Required core-file size %lu bytes is greater than hard limit %lu bytes\n",
                bytes, (unsigned long)lim.rlim_max);
        return;
    }

    lim.rlim_cur = bytes;

    if (setrlimit(RLIMIT_CORE, &lim) == -1)
    {
        fprintf(stderr, "Error changing core-file size: %s\n", strerror(errno));
    }
}

// -d
void printfl_d()
{
    char pwd[1024];
    if (getcwd(pwd, sizeof(pwd)) != NULL)
    {
        printf("Current working directory: %s\n", pwd);
        printf("--------------------------------------\n");
    }
    else
    {
        fprintf(stderr, "Error getting current working directory: %s\n", strerror(errno));
    }
}

// -v
void printfl_v()
{
    printf("Enviroment variables:\n");
    for (char **env = environ; *env != NULL; env++)
    {
        printf("  %s\n", *env);
    }
    printf("--------------------------------------\n");
}

// -V
void printfl_V(char *env_string)
{
    if (strchr(env_string, '=') == NULL)
    {
        fprintf(stderr, "Error: try using -Vname=value\n");
        return;
    }

    if (putenv(env_string) != 0)
    {
        fprintf(stderr, "Error changing env variable: %s\n", strerror(errno));
    }
}

int main(int argc, char *argv[])
{
    char options[] = "ispuU:cC:dvV:"; /* valid options */
    int opt;
    printf("argc equals %d\n", argc);

    while ((opt = getopt(argc, argv, options)) != EOF)
    {
        switch (opt)
        {
        case 'i':
            printfl_i();
            break;

        case 's':
            if (setpgid(0, 0) == -1)
            {
                perror("Error calling setpgid\n");
                return 1;
            }

            break;

        case 'p':
            printfl_p();
            break;

        case 'u':
            printfl_u();
            break;

        case 'U':
            printfl_U(atol(optarg));
            break;

        case 'c':
            printfl_c();
            break;

        case 'C':
            printfl_C((unsigned long)atol(optarg));
            break;

        case 'd':
            printfl_d();
            break;

        case 'v':
            printfl_v();
            break;

        case 'V':
            printfl_V(optarg);
            break;

        case '?':
            break;

        default:
            fprintf(stderr, "Illegal option -- %c\n", opt);
        }
    }
}