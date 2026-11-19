#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <errno.h>

int main(void)
{
    int channel[2];
    pid_t pid;
    int status;
    char buffer[128];
    ssize_t count;
    ssize_t result;
    size_t i;

    const char message[] = "Hello!!! My name is Nika, im 18 y.o.\n";

    // создание канала для передачи данных между процессами
    if (pipe(channel) == -1) {
        perror("pipe");
        return 1;
    }

    // создание дочернего процесса
    pid = fork();

    if (pid == -1) {
        perror("fork");
        close(channel[0]);
        close(channel[1]);
        return 1;
    }

    if (pid == 0) {
        // ребенок будет только читать, поэтому закрываю конец для записи
        close(channel[1]);

        // чтение текста из канала, пока родитель не закончит передачу
        while ((count = read(channel[0], buffer, sizeof(buffer))) > 0) {

            // преобразование всех английских буквы в верхний регистр
            for (i = 0; i < (size_t)count; i++) {
                buffer[i] = (char)toupper((unsigned char)buffer[i]);
            }

            // вывод преобразованного текста на экран
            if (fwrite(buffer, 1, (size_t)count, stdout) !=
                (size_t)count) {
                perror("fwrite");
                close(channel[0]);
                _exit(1);
            }
        }

        // проверка, не произошла ли ошибка чтения
        if (count == -1) {
            perror("read");
            close(channel[0]);
            _exit(1);
        }

        // освобождение канала и завершение дочернего процесса
        close(channel[0]);

        if (fflush(stdout) == EOF) {
            _exit(1);
        }

        _exit(0);
    }

    // родитель будет только записывать, закрываю конец для чтения
    close(channel[0]);

    // передача текста дочернему процессу через канал
    result = write(channel[1], message, sizeof(message) - 1);

    if (result != (ssize_t)(sizeof(message) - 1)) {
        fprintf(stderr, "failed to send message\n");
        close(channel[1]);
        waitpid(pid, NULL, 0);
        return 1;
    }

    // закрываю канал, чтобы ребенок получил признак конца данных
    close(channel[1]);

    // ожидание завершения дочернего процесса
    do {
        result = waitpid(pid, &status, 0);
    } while (result == -1 && errno == EINTR);

    if (result == -1) {
        perror("waitpid");
        return 1;
    }

    // проверка, успешно ли завершился ребенок
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        fprintf(stderr, "child process failed\n");
        return 1;
    }

    return 0;
}