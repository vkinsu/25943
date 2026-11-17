#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <string.h>

#define MAX_OPTS 100

struct opt_rec {
    int opt;
    char *arg;
};

int main(int argc, char *argv[])
{
    struct opt_rec opts[MAX_OPTS];
    int nopts = 0;
    int opt;
    int i;
    struct rlimit rl;

    /* Собираем опции слева направо */
    while ((opt = getopt(argc, argv, "ispuU:cC:dvV:")) != -1) {
        if (nopts >= MAX_OPTS) {
            fprintf(stderr, "Too many options\n");
            return 1;
        }
        opts[nopts].opt = opt;
        opts[nopts].arg = optarg;
        nopts++;
    }

    /* Обрабатываем справа налево */
    for (i = nopts - 1; i >= 0; i--) {
        switch (opts[i].opt) {
        case 'i':
            printf("-i: real uid=%d, effective uid=%d\n", getuid(), geteuid());
            printf("-i: real gid=%d, effective gid=%d\n", getgid(), getegid());
            break;

        case 's':
            if (setpgid(0, 0) == 0)
                printf("-s: became process group leader, pgid=%d\n", getpgrp());
            else
                perror("-s: setpgid");
            break;

        case 'p':
            printf("-p: pid=%d, ppid=%d, pgid=%d\n",
                   getpid(), getppid(), getpgrp());
            break;

        case 'u':
            if (getrlimit(RLIMIT_FSIZE, &rl) == 0)
                printf("-u: ulimit (max file size) = %ld\n", (long)rl.rlim_cur);
            else
                perror("-u: getrlimit");
            break;

        case 'U':
            rl.rlim_cur = atol(opts[i].arg);
            rl.rlim_max = rl.rlim_cur;
            if (setrlimit(RLIMIT_FSIZE, &rl) == 0)
                printf("-U: new ulimit = %ld\n", (long)rl.rlim_cur);
            else
                perror("-U: setrlimit");
            break;

        case 'c':
            if (getrlimit(RLIMIT_CORE, &rl) == 0)
                printf("-c: core size = %ld\n", (long)rl.rlim_cur);
            else
                perror("-c: getrlimit");
            break;

        case 'C':
            rl.rlim_cur = atol(opts[i].arg);
            rl.rlim_max = rl.rlim_cur;
            if (setrlimit(RLIMIT_CORE, &rl) == 0)
                printf("-C: new core size = %ld\n", (long)rl.rlim_cur);
            else
                perror("-C: setrlimit");
            break;

        case 'd': {
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd)) != NULL)
                printf("-d: cwd = %s\n", cwd);
            else
                perror("-d: getcwd");
            break;
        }

        case 'v': {
            extern char **environ;
            char **env = environ;
            printf("-v: environment:\n");
            while (*env != NULL) {
                printf("    %s\n", *env);
                env++;
            }
            break;
        }

        case 'V':
            {
                char *eq = strchr(opts[i].arg, '=');
                if (eq == NULL) {
                    fprintf(stderr, "-V: expected name=value\n");
                    break;
                }
                *eq = '\0';
                if (setenv(opts[i].arg, eq + 1, 1) == 0)
                    printf("-V: set %s=%s\n", opts[i].arg, eq + 1);
                else
                    perror("-V: setenv");
            }
            break;

        default:
            fprintf(stderr, "Unknown option\n");
        }
    }

    return 0;
}

