#define __EXTENSIONS__
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <pwd.h>
#include <grp.h>

extern char **environ;

void print_i(void) {
    uid_t ruid = getuid();
    uid_t euid = geteuid();
    gid_t rgid = getgid();
    gid_t egid = getegid();
    printf("Real UID: %d, Effective UID: %d\n", (int)ruid, (int)euid);
    printf("Real GID: %d, Effective GID: %d\n", (int)rgid, (int)egid);
}

void print_s(void) {
    if (setpgid(0, 0) == -1) {
        perror("setpgid");
    } else {
        printf("Process became process group leader. PID: %d\n", (int)getpid());
    }
}

void print_p(void) {
    printf("Process ID: %d\n", (int)getpid());
    printf("Parent Process ID: %d\n", (int)getppid());
    printf("Process Group ID: %d\n", (int)getpgrp());
}

void print_u(void) {
    FILE *fp = popen("ulimit -u", "r");
    if (fp != NULL) {
        char buf[128];
        if (fgets(buf, sizeof(buf), fp) != NULL) {
            buf[strcspn(buf, "\r\n")] = 0;
            printf("ulimit: %s\n", buf);
        }
        pclose(fp);
    } else {
        perror("popen");
    }
}

void set_u(const char *val_str) {
    long new_val = atol(val_str);
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "ulimit -u %ld", new_val);
    if (system(cmd) != 0) {
        fprintf(stderr, "Failed to set ulimit (system restriction)\n");
    } else {
        printf("New ulimit set to %ld\n", new_val);
    }
}

void print_c(void) {
    struct rlimit rlim;
    if (getrlimit(RLIMIT_CORE, &rlim) == 0) {
        if (rlim.rlim_cur == RLIM_INFINITY) {
            printf("core size: unlimited\n");
        } else {
            printf("core size: %lu\n", (unsigned long)rlim.rlim_cur);
        }
    } else {
        perror("getrlimit");
    }
}

void set_c(const char *val_str) {
    rlim_t new_val = (rlim_t)atol(val_str);
    struct rlimit rlim;
    if (getrlimit(RLIMIT_CORE, &rlim) == 0) {
        rlim.rlim_cur = new_val;
        if (setrlimit(RLIMIT_CORE, &rlim) == -1) {
            perror("setrlimit");
        } else {
            printf("New core size set to %lu\n", (unsigned long)new_val);
        }
    } else {
        perror("getrlimit");
    }
}

void print_d(void) {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Current working directory: %s\n", cwd);
    } else {
        perror("getcwd");
    }
}

void print_v(void) {
    char **env = environ;
    while (*env) {
        printf("%s\n", *env);
        env++;
    }
}

void set_env(const char *assignment) {
    if (putenv((char *)assignment) != 0) {
        perror("putenv");
    } else {
        printf("Environment variable updated: %s\n", assignment);
    }
}

int main(int argc, char *argv[]) {
    int opt;
    opterr = 0;

    for (int i = argc - 1; i > 0; i--) {
        if (argv[i][0] == '-') {
            char flag = argv[i][1];
            if (flag == 'U' && argv[i][2] != '\0') {
                set_u(&argv[i][2]);
            } else if (flag == 'C' && argv[i][2] != '\0') {
                set_c(&argv[i][2]);
            } else if (flag == 'V') {
                set_env(&argv[i][2]);
            }
        }
    }

    while ((opt = getopt(argc, argv, "ispuU:cC:dvV:")) != -1) {
        switch (opt) {
            case 'i':
                print_i();
                break;
            case 's':
                print_s();
                break;
            case 'p':
                print_p();
                break;
            case 'u':
                print_u();
                break;
            case 'U':
                break;
            case 'c':
                print_c();
                break;
            case 'C':
                break;
            case 'd':
                print_d();
                break;
            case 'v':
                print_v();
                break;
            case 'V':
                break;
            default:
                fprintf(stderr, "Invalid option: -%c\n", optopt);
                break;
        }
    }

    return 0;
}
