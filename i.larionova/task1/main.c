#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/resource.h> // RLIMIT_FSIZE...
#include <errno.h>

extern char **environ;

static void print_fsize_limit(void)
{
    struct rlimit lim;
    if (getrlimit(RLIMIT_FSIZE, &lim) == -1) // ← Поменяй RLIMIT_NPROC на RLIMIT_FSIZE
    {
        fprintf(stderr, "ulimit reading error: %s\n", strerror(errno));
        return;
    }

    printf("Soft limit: %ld, hard limit: %ld\n",
           (long)lim.rlim_cur, (long)lim.rlim_max);
    printf("\n");
}

static void set_fsize_limit(long new_soft_ulimit)
{
    struct rlimit lim;

    if (getrlimit(RLIMIT_FSIZE, &lim) == -1) // ← Поменяй RLIMIT_NPROC на RLIMIT_FSIZE
    {
        fprintf(stderr, "ulimit reading error: %s\n", strerror(errno));
        return;
    }

    if ((rlim_t)new_soft_ulimit > lim.rlim_max)
    {
        fprintf(stderr, "Error: new ulimit %ld is greater than hard limit %ld\n",
                (long)new_soft_ulimit, (long)lim.rlim_max);
        return;
    }

    lim.rlim_cur = (rlim_t)new_soft_ulimit;

    if (setrlimit(RLIMIT_FSIZE, &lim) == -1) // ← Поменяй RLIMIT_NPROC на RLIMIT_FSIZE
    {
        fprintf(stderr, "Error changing ulimit: %s\n", strerror(errno));
        return;
    }
}

static void print_core_limit(void)
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
    printf("\n");
}

static void set_core_limit(const char *arg)
{
    struct rlimit rl;
    if (getrlimit(RLIMIT_CORE, &rl) == -1) { perror("getrlimit"); return; }
    rl.rlim_cur = (rlim_t)atol(arg);
    if (setrlimit(RLIMIT_CORE, &rl) == -1) perror("setrlimit");
}

static void print_cwd(void)
{
    char buf[PATH_MAX];
    if (getcwd(buf, sizeof(buf)) == NULL)
        perror("getcwd");
    else
        printf("cwd = %s\n", buf);
}

static void print_env(void)
{
    for (char **e = environ; *e != NULL; e++)
        printf("%s\n", *e);
}

static void set_env_var(char *arg)
{
    char *eq = strchr(arg, '=');
    if (eq == NULL) 
    {
        fprintf(stderr, "-V требует формат name=value\n");
        return;
    }
    *eq = '\0';
    if (setenv(arg, eq + 1, 1) == -1)
        perror("setenv");
    *eq = '=';
}

int  main(int argc, char *argv[])
{
    char options[ ] = "ispuU:cC:dvV:";  /* valid options */
    printf("argc equals %d\n", argc);
    
    // ПЕРЕВОРАЧИВАЕМ argv[1..argc-1], чтобы getopt шёл справа налево
    for (int i = 1, j = argc - 1; i < j; i++, j--) {
        char *tmp = argv[i];
        argv[i] = argv[j];
        argv[j] = tmp;
    }

    int c;
    while ((c = getopt(argc, argv, options)) != EOF) 
    {
        switch (c) 
        {
            case 'i':
                printf("real uid = %d, effective uid = %d\n", getuid(), geteuid());
                printf("real gid = %d, effective gid = %d\n", getgid(), getegid());
                break;
            case 's':
                // setpgid(pid, pgid): делает процесс pid лидером группы pgid
                if (setpgid(0, 0) == -1)
                    perror("setpgid");
                else
                    printf("новый лидер группы, pgid = %d\n", getpgrp());
                break;
            case 'p':
                printf("pid = %d, ppid = %d, pgid = %d\n", getpid(), getppid(), getpgrp());
                break;
            case 'u': 
                print_fsize_limit(); 
                break;
            case 'U': 
                set_fsize_limit(atol(optarg)); 
                break;
            case 'c': 
                print_core_limit();  
                break;
            case 'C': 
                set_core_limit(optarg); 
                break;
            case 'd': 
                print_cwd();         
                break;
            case 'v': 
                print_env();
                break;
            case 'V': 
                set_env_var(optarg);
                break;
            case '?':
            default:
                fprintf(stderr, "неизвестная опция: -%c\n", optopt);
                return 1;
        }
    }
}
