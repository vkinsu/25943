#include <stdio.h>
#include <signal.h>
#include <unistd.h>

volatile sig_atomic_t count = 0;
volatile sig_atomic_t quit = 0;

void handle_sigint(int sig)
{
    (void)sig;
    count++;
    write(STDOUT_FILENO, "\a", 1);
}

void handle_sigquit(int sig)
{
    (void)sig;
    quit = 1;
}

int main(void)
{
    struct sigaction sa;

    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    sa.sa_handler = handle_sigint;
    sigaction(SIGINT, &sa, NULL);

    sa.sa_handler = handle_sigquit;
    sigaction(SIGQUIT, &sa, NULL);

    printf("Program started.\n");
    printf("Press Ctrl+C to beep, Ctrl+\\ to exit.\n");
    fflush(stdout);

    while (!quit)
    {
        pause();
    }

    printf("\nNumber of beeps: %d\n", (int)count);
    printf("Program finished.\n");

    return 0;
}