#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <signal.h>
#include <unistd.h>

int main(void)
{
    sigset_t signals;
    int received_signal;
    int signal_count = 0;
    int result;

    // создание набора сигналов, которые будут обрабатываться
    sigemptyset(&signals);
    sigaddset(&signals, SIGINT);
    sigaddset(&signals, SIGQUIT);

    // блокирую обычную обработку сигналов
    // теперь программа сможет получать их через sigwait
    if (sigprocmask(SIG_BLOCK, &signals, NULL) == -1) {
        perror("sigprocmask");
        return 1;
    }

    printf("Press Ctrl-C to beep, Ctrl-\\ to quit.\n");
    fflush(stdout);

    // бесконечное ожидание сигналов от терминала
    for (;;) {
        result = sigwait(&signals, &received_signal);

        if (result != 0) {
            fprintf(stderr, "sigwait error: %d\n", result);
            return 1;
        }

        if (received_signal == SIGINT) {
            // при ctrl-c будет подача звукового сигнала и увеличение счетчика
            if (write(STDOUT_FILENO, "\a", 1) != 1) {
                perror("write");
                return 1;
            }

            signal_count++;
        } else if (received_signal == SIGQUIT) {
            // при ctrl-\ вывод количества сигналов и завершение
            printf("\nBell count: %d\n", signal_count);
            break;
        }
    }

    return 0;
}