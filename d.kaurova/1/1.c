#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/types.h>

extern char **environ;

void do_i(void)
{
    printf("uid=%d euid=%d gid=%d egid=%d\n",
           getuid(), geteuid(), getgid(), getegid());
}

void do_s(void)
{
    if (setpgid(0, 0) == -1) {
        perror("setpgid");
        return;
    }
    printf("pid=%d pgid=%d\n", getpid(), getpgrp());
}

void do_p(void)
{
    printf("pid=%d ppid=%d pgid=%d\n",
           getpid(), getppid(), getpgrp());
}

void do_u(void)
{
    FILE *fp;
    char buf[64];

    fp = popen("ulimit -u", "r");
    if (fp == NULL) {
        perror("popen");
        return;
    }

    if (fgets(buf, sizeof(buf), fp) != NULL) {
        printf("ulimit= %s", buf);
        if (buf[0] != '\0' && buf[strlen(buf) - 1] != '\n')
            printf("\n");
    }

    pclose(fp);
}

void do_U(const char *arg)
{
    struct rlimit rl;

    if (getrlimit(RLIMIT_FSIZE, &rl) == -1) {
        perror("getrlimit");
        return;
    }

    rl.rlim_cur = atol(arg);

    if (setrlimit(RLIMIT_FSIZE, &rl) == -1) {
        perror("setrlimit");
        return;
    }

    if (getrlimit(RLIMIT_FSIZE, &rl) == -1) {
        perror("getrlimit");
        return;
    }

    if (rl.rlim_cur == RLIM_INFINITY)
        printf("ulimit=unlimited\n");
    else
        printf("ulimit=%ld\n", (long)rl.rlim_cur);
}

void do_c(void)
{
    struct rlimit rl;

    if (getrlimit(RLIMIT_CORE, &rl) == -1) {
        perror("getrlimit");
        return;
    }

    if (rl.rlim_cur == RLIM_INFINITY)
        printf("core=unlimited\n");
    else
        printf("core=%ld\n", (long)rl.rlim_cur);
}

void do_C(const char *arg)
{
    struct rlimit rl;

    if (getrlimit(RLIMIT_CORE, &rl) == -1) {
        perror("getrlimit");
        return;
    }

    rl.rlim_cur = atol(arg);

    if (setrlimit(RLIMIT_CORE, &rl) == -1) {
        perror("setrlimit");
        return;
    }

    if (getrlimit(RLIMIT_CORE, &rl) == -1) {
        perror("getrlimit");
        return;
    }

    if (rl.rlim_cur == RLIM_INFINITY)
        printf("core=unlimited\n");
    else
        printf("core=%ld\n", (long)rl.rlim_cur);
}

void do_d(void)
{
    char buf[4096];

    if (getcwd(buf, sizeof(buf)) == NULL) {
        perror("getcwd");
        return;
    }

    printf("cwd=%s\n", buf);
}

void do_v(void)
{
    char **e;

    for (e = environ; *e != NULL; e++)
        printf("%s\n", *e);
}

void do_V(const char *arg)
{
    if (putenv((char *)arg) != 0) {
        perror("putenv");
        return;
    }
}

int main(int argc, char *argv[])
{
    int opt;

    while ((opt = getopt(argc, argv, "ispuU:cC:dvV:")) != -1) {
        switch (opt) {
            case 'i': do_i();       break;
            case 's': do_s();       break;
            case 'p': do_p();       break;
            case 'u': do_u();       break;
            case 'U': do_U(optarg); break;
            case 'c': do_c();       break;
            case 'C': do_C(optarg); break;
            case 'd': do_d();       break;
            case 'v': do_v();       break;
            case 'V': do_V(optarg); break;
            default:
                fprintf(stderr, "bad option: -%c\n", optopt);
                break;
        }
    }

    return 0;
}
