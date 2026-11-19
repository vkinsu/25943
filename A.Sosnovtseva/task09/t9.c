#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void)
{
    pid_t child = fork();

    if (child == -1) {
        perror("fork");
        return 1;
    }

    if (child == 0) {
        execlp("cat", "cat", "text.txt", (char *)NULL);
        perror("cat");
        _exit(1);
    }

    printf("Parent: started.\n");

    if (waitpid(child, NULL, 0) == -1) {
        perror("waitpid");
        return 1;
    }

    printf("Parent: child finished.\n");
    return 0;
}