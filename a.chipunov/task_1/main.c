#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ulimit.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/resource.h>

extern char **environ;

static long parse_num(const char *s)
{
    char *end;
    long v = strtol(s, &end, 10);
    if (end == s || *end != '\0' || v < 0)
        return -1;
    return v;
}

int main(int argc, char *argv[])
{
    char opts[] = "ispuU:cC:dvV:";
    int c, i, n = 0, total = 0;
    char *flags;
    char **args;
    struct rlimit rl;
    char cwd[PATH_MAX];
    char **e;
    long v;

    if (argc < 2) {
        fprintf(stderr, "usage: %s [-i] [-s] [-p] [-u] [-Unew_ulimit] "
                "[-c] [-Csize] [-d] [-v] [-Vname=value]\n", argv[0]);
        return 0;
    }

    /* upper bound for number of options */
    for (i = 1; i < argc; i++)
        total += strlen(argv[i]);
    flags = malloc(total);
    args = malloc(total * sizeof(char *));
    if (flags == NULL || args == NULL) {
        perror("malloc");
        return 1;
    }

    /* collect options */
    while ((c = getopt(argc, argv, opts)) != -1) {
        if (c == '?')
            continue;   /* getopt already printed error */
        flags[n] = c;
        args[n] = optarg;
        n++;
    }

    /* process right to left */
    for (i = n - 1; i >= 0; i--) {
        switch (flags[i]) {
        case 'i':
            printf("uid=%ld euid=%ld gid=%ld egid=%ld\n",
                   (long)getuid(), (long)geteuid(),
                   (long)getgid(), (long)getegid());
            break;
        case 's':
            if (setpgid(0, 0) == -1)
                perror("setpgid");
            break;
        case 'p':
            printf("pid=%ld ppid=%ld pgid=%ld\n",
                   (long)getpid(), (long)getppid(), (long)getpgrp());
            break;
        case 'u':
                if (getrlimit(RLIMIT_NPROC, &rl) == -1)
                    perror("getrlimit");
                else if (rl.rlim_cur == RLIM_INFINITY)
                    printf("max processes=unlimited\n");
                else
                    printf("max processes=%lu\n", (unsigned long)rl.rlim_cur);
                break;
        case 'U':
            v = parse_num(args[i]);
            if (v == -1)
                fprintf(stderr, "bad ulimit value: %s\n", args[i]);
            else if (ulimit(UL_SETFSIZE, v) == -1)
                perror("ulimit");
            break;
        case 'c':
            if (getrlimit(RLIMIT_CORE, &rl) == -1)
                perror("getrlimit");
            else if (rl.rlim_cur == RLIM_INFINITY)
                printf("core size=unlimited\n");
            else
                printf("core size=%lu\n", (unsigned long)rl.rlim_cur);
            break;
        case 'C':
            v = parse_num(args[i]);
            if (v == -1) {
                fprintf(stderr, "bad core size: %s\n", args[i]);
                break;
            }
            if (getrlimit(RLIMIT_CORE, &rl) == -1) {
                perror("getrlimit");
                break;
            }
            rl.rlim_cur = v;
            if (setrlimit(RLIMIT_CORE, &rl) == -1)
                perror("setrlimit");
            break;
        case 'd':
            if (getcwd(cwd, sizeof(cwd)) == NULL)
                perror("getcwd");
            else
                printf("cwd=%s\n", cwd);
            break;
        case 'v':
            for (e = environ; *e != NULL; e++)
                printf("%s\n", *e);
            break;
        case 'V':
            if (strchr(args[i], '=') == NULL)
                fprintf(stderr, "bad -V value: %s (need name=value)\n", args[i]);
            else if (putenv(args[i]) != 0)
                perror("putenv");
            break;
        }
    }

    free(flags);
    free(args);
    return 0;
}
