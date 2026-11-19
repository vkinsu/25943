#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    const char *filename;
    pid_t child_pid;
    pid_t wait_result;
    int status;

    // выбираю файл для команды cat
    if (argc > 2) {
        fprintf(stderr, "usage: %s [filename]\n", argv[0]);
        return 1;
    }

    filename = (argc == 2) ? argv[1] : "test.txt";

    // создание дочернего процесса
    child_pid = fork();

    if (child_pid == -1) {
        perror("fork");
        return 1;
    }

    if (child_pid == 0) {
        // дочерний процесс заменяется командой cat
        execlp("cat", "cat", filename, (char *)NULL);

        // сюда попадем только при ошибке execlp
        perror("execlp");
        _exit(127);
    }

    // родительский процесс выводит свое сообщение
    printf("parent: child process started\n");
    fflush(stdout);

    // ожидание завершения дочернего процесса
    do {
        wait_result = waitpid(child_pid, &status, 0);
    } while (wait_result == -1 && errno == EINTR);

    if (wait_result == -1) {
        perror("waitpid");
        return 1;
    }

    // последнее сообщение выводится только после завершения ребенка
    printf("parent: child process finished\n");

    // проверка успешности завершения дочернего процесса
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status) == 0 ? 0 : 1;
    }

    return 1;
}