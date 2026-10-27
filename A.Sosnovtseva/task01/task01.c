#include <sys/types.h>
#include <sys/resource.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ulimit.h>

extern char **environ;

static int error(const char *name)
{
    perror(name);
    return -1;
}

static int number(const char *s, long *value)
{
    char *end;

    errno = 0;
    *value = strtol(s, &end, 10);
    return *s >= '0' && *s <= '9' && !*end && errno == 0;
}

static int options(int argc, char **argv)
{
    int opt = getopt(argc, argv, ":ispuU:cC:dvV:");
    char *arg = optarg;
    struct rlimit limit;
    char path[PATH_MAX + 1];
    long value;

    if (opt == -1) {
        if (optind == argc)
            return 0;
        fprintf(stderr, "Unexpected argument: %s\n", argv[optind]);
        return -1;
    }

    if (opt == '?' || opt == ':') {
        fprintf(stderr, opt == '?' ? "Unknown option: -%c\n" :
                "Option -%c needs an argument\n", optopt);
        return -1;
    }

    if (options(argc, argv) == -1)
        return -1;

    switch (opt) {
    case 'i':
        printf("uid=%lu euid=%lu gid=%lu egid=%lu\n",
               (unsigned long)getuid(), (unsigned long)geteuid(),
               (unsigned long)getgid(), (unsigned long)getegid());
        break;
    case 's':
        if (setpgid(0, 0) == -1)
            return error("setpgid");
        break;
    case 'p':
        printf("pid=%ld ppid=%ld pgrp=%ld\n",
               (long)getpid(), (long)getppid(), (long)getpgrp());
        break;
    case 'u':
        errno = 0;
        value = ulimit(UL_GETFSIZE);
        if (value == -1 && errno)
            return error("ulimit");
        printf("ulimit=%ld\n", value);
        break;
    case 'U':
    case 'C':
        if (!number(arg, &value)) {
            fprintf(stderr, "Bad value for -%c: %s\n", opt, arg);
            return -1;
        }
        if (opt == 'U') {
            if (ulimit(UL_SETFSIZE, value) == -1)
                return error("ulimit");
        } else {
            if (getrlimit(RLIMIT_CORE, &limit) == -1)
                return error("getrlimit");
            limit.rlim_cur = (rlim_t)value;
            if (setrlimit(RLIMIT_CORE, &limit) == -1)
                return error("setrlimit");
        }
        break;
    case 'c':
        if (getrlimit(RLIMIT_CORE, &limit) == -1)
            return error("getrlimit");
        if (limit.rlim_cur == RLIM_INFINITY)
            puts("core=unlimited");
        else
            printf("core=%llu\n", (unsigned long long)limit.rlim_cur);
        break;
    case 'd':
        if (!getcwd(path, sizeof path))
            return error("getcwd");
        puts(path);
        break;
    case 'v':
        for (char **p = environ; *p; ++p)
            puts(*p);
        break;
    case 'V':
        if (!strchr(arg, '=') || *arg == '=') {
            fprintf(stderr, "Expected name=value: %s\n", arg);
            return -1;
        }
        if (putenv(arg) != 0)
            return error("putenv");
        break;
    }
    return 0;
}

int main(int argc, char **argv)
{
    opterr = 0;
    return options(argc, argv) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}