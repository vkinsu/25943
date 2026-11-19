#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

int main(void)
{
    pid_t pid;

    // дочерний процесс
    pid = fork();

    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        // дочерний процесс запускает cat
        execlp("cat", "cat", "test.txt", (char *)NULL);

        perror("execlp");
        _exit(127);
    }

    // родитель не ждет завершения дочернего процесса
    printf("parent: child process started\n");
    printf("parent: child process finished\n");

    return 0;
}