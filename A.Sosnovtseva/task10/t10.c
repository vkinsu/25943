#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char **argv)
{
    pid_t child;
    int result;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s command... \n", argv[0]);
        return 1;
    }

    child = fork();
    switch (child) {
    case -1:
        perror("fork");
        return 1;
    case 0:
        execvp(argv[1], argv + 1);
        perror(argv[1]);
        _exit(1);
    }

    while (waitpid(child, &result, 0) == -1) {
        if (errno == EINTR)
            continue;
        perror("waitpid");
        return 1;
    }

    if (WIFEXITED(result))
        printf("Exit code: %d\n", WEXITSTATUS(result));
    else if (WIFSIGNALED(result))
        printf("Terminated by signal: %d\n", WTERMSIG(result));

    return 0;
}