#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    pid_t child_pid;
    int status;

    // проверка, указана ли команда
    if (argc < 2) {
        fprintf(stderr, "usage: %s command [arguments]\n", argv[0]);
        return 1;
    }

    // создание дочернего процесса
    child_pid = fork();

    if (child_pid == -1) {
        perror("fork");
        return 1;
    }

    if (child_pid == 0) {
        // запускаем переданную команду со всеми ее аргументами
        execvp(argv[1], argv + 1);

        // если команда не запустилась, вывод ошибки
        perror("execvp");
        _exit(127);
    }

    // родитель ждет завершения дочернего процесса
    while (waitpid(child_pid, &status, 0) == -1) {
        if (errno != EINTR) {
            perror("waitpid");
            return 1;
        }
    }

    // определяем, как завершилась команда
    if (WIFEXITED(status)) {
        printf("exit code: %d\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("terminated by signal: %d\n", WTERMSIG(status));
    }

    return 0;
}