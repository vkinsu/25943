#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <limits.h>

extern char **environ;

int main(int argc, char *argv[])
{
    int opt;
    char options[100];
    char *args[100];
    int n = 0;

    while ((opt = getopt(argc, argv, "ispuU:cC:dV:v")) != -1)
    {
        options[n] = opt;
        args[n] = optarg;
        n++;
    }

    for (int i = n - 1; i >= 0; i--)
    {
        switch (options[i])
        {
            case 'i':
                printf("UID: %d EUID: %d\n", getuid(), geteuid());
                printf("GID: %d EGID: %d\n", getgid(), getegid());
                break;

            case 's':
                if (setpgid(0, 0) == -1)
                    perror("setpgid");
                break;

            case 'p':
                printf("PID: %d PPID: %d PGID: %d\n",
                       getpid(), getppid(), getpgrp());
                break;

            case 'u':
            {
                struct rlimit r;

                if (getrlimit(RLIMIT_FSIZE, &r) == -1)
                    perror("getrlimit");
                else
                {
                    if (r.rlim_cur == RLIM_INFINITY)
                        printf("ulimit: unlimited\n");
                    else
                        printf("ulimit: %ld\n", (long)r.rlim_cur);
                }

                break;
            }

            case 'U':
            {
                struct rlimit r;

                if (getrlimit(RLIMIT_FSIZE, &r) == -1)
                    perror("getrlimit");
                else
                {
                    r.rlim_cur = atol(args[i]);

                    if (setrlimit(RLIMIT_FSIZE, &r) == -1)
                        perror("setrlimit");
                }

                break;
            }

            case 'c':
            {
                struct rlimit r;

                if (getrlimit(RLIMIT_CORE, &r) == -1)
                    perror("getrlimit");
                else
                {
                    if (r.rlim_cur == RLIM_INFINITY)
                        printf("Core size: unlimited\n");
                    else
                        printf("Core size: %ld\n", (long)r.rlim_cur);
                }

                break;
            }

            case 'C':
            {
                struct rlimit r;

                if (getrlimit(RLIMIT_CORE, &r) == -1)
                    perror("getrlimit");
                else
                {
                    r.rlim_cur = atol(args[i]);

                    if (setrlimit(RLIMIT_CORE, &r) == -1)
                        perror("setrlimit");
                }

                break;
            }

            case 'd':
            {
                char cwd[PATH_MAX];

                if (getcwd(cwd, sizeof(cwd)) == NULL)
                    perror("getcwd");
                else
                    printf("Directory: %s\n", cwd);

                break;
            }

            case 'v':
                for (char **env = environ; *env != NULL; env++)
                    printf("%s\n", *env);
                break;

            case 'V':
                if (putenv(args[i]) != 0)
                    perror("putenv");
                break;

            default:
                fprintf(stderr, "Unknown option\n");
                return 1;
        }
    }

    return 0;
}

