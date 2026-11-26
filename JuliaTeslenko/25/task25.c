
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>

int main(void)
{
    int fd[2];

    if (pipe(fd) == -1) 
    {
        perror("pipe");
        return 1;
    }

    pid_t pid = fork();

    if (pid == -1) 
    {
        perror("fork");
        return 1;
    }

    if (pid == 0) 
    {
        // Дочерний процесс

        close(fd[1]);

        char ch;

        printf("Uppercase text: ");

        while (read(fd[0], &ch, 1) > 0) 
        {
            putchar(toupper((unsigned char)ch));
        }

        printf("\n");

        close(fd[0]);
    }
    else 
    {
        // Родительский процесс

        close(fd[0]);

        char text[] = "Hello World! This is a Pipe.";

        printf("Original text: %s\n", text);

        write(fd[1], text, sizeof(text) - 1);

        close(fd[1]);

        wait(NULL);
    }

    return 0;
}
