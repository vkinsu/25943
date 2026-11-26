#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

int main()
{
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("fork");
        return 1;
    }

    if (pid == 0)
    {
        execlp("cat", "cat", "longfile.txt", (char *)NULL);
        perror("execlp");
        exit(1);
    }

    else
    {
        printf("Parrent process: Hello!\n");
        fflush(stdout);

        wait(NULL);

        printf("Child process has finished.\n");
    }

    return 0;
}