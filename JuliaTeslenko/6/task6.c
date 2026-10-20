#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>

#define MAX_LINES 1000
#define MAX_LEN 1024

typedef struct {
    off_t offset;
    off_t length;
} Line;

int fd;


void handle_sigint(int sig)
{
    (void)sig;

    printf("\nCtrl+C pressed. Program continues.\n");
}


void handle_sigalrm(int sig)
{
    char buffer[1024];
    ssize_t bytes_read;

    (void)sig;

    printf("\nTime is up!\n");
    printf("File contents:\n");

    lseek(fd, 0L, SEEK_SET);

    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
    {
        write(STDOUT_FILENO, buffer, bytes_read);
    }

    close(fd);
    exit(0);
}


int main(int argc, char *argv[])
{
    char c;
    Line lines[MAX_LINES];
    int cnt = 0;
    off_t line_start = 0;
    off_t pos;

    signal(SIGINT, handle_sigint);
    signal(SIGALRM, handle_sigalrm);

    if (argc != 2)
    {
        printf("Usage: %s filename\n", argv[0]);
        return 1;
    }

    fd = open(argv[1], O_RDONLY);

    if (fd == -1)
    {
        perror("open");
        return 1;
    }

    while (read(fd, &c, 1) == 1)
    {
        if (c == '\n')
        {
            pos = lseek(fd, 0L, SEEK_CUR);

            lines[cnt].offset = line_start;
            lines[cnt].length = pos - line_start - 1;

            cnt++;

            line_start = pos;

            if (cnt >= MAX_LINES)
            {
                break;
            }
        }
    }

    pos = lseek(fd, 0L, SEEK_CUR);

    if (line_start < pos && cnt < MAX_LINES)
    {
        lines[cnt].offset = line_start;
        lines[cnt].length = pos - line_start;
        cnt++;
    }


    printf("\nLine table:\n");
    printf("Line\tOffset\tLength\n");

    for (int i = 0; i < cnt; i++)
    {
        printf("%d\t%lld\t%lld\n",
               i + 1,
               (long long)lines[i].offset,
               (long long)lines[i].length);
    }

    int first_request = 1;


    while (1)
    {
        char input[100];
        int pos_input = 0;
        int valid = 1;
        int number;

        printf("\nEnter line number (0 - exit): ");
        fflush(stdout);

        if (first_request)
        {
            alarm(5);
        }


        while (pos_input < 99)
        {
            char ch;

            if (read(STDIN_FILENO, &ch, 1) != 1)
            {
                break;
            }

            if (ch == '\n')
            {
                break;
            }

            if ((unsigned char)ch == 3)
            {
                printf("\nCtrl+C pressed.\n");
                valid = 0;
                continue;
            }

            if (iscntrl((unsigned char)ch))
            {
                valid = 0;
                continue;
            }

            if (!isdigit((unsigned char)ch))
            {
                valid = 0;
                continue;
            }

            input[pos_input] = ch;
            pos_input++;
        }

        if (first_request)
        {
            alarm(0);
        }


        input[pos_input] = '\0';


        if (!valid || pos_input == 0)
        {
            printf("Please enter a number.\n");

            first_request = 0;

            continue;
        }


        number = atoi(input);


        if (number == 0)
        {
            break;
        }


        if (number < 1 || number > cnt)
        {
            printf("No such line.\n");

            first_request = 0;

            continue;
        }

        first_request = 0;

        if (lseek(fd, lines[number - 1].offset, SEEK_SET) == -1)
        {
            perror("lseek");
            continue;
        }


        char buffer[MAX_LEN];

        if (lines[number - 1].length >= MAX_LEN)
        {
            printf("The string is too long.\n");
            continue;
        }


        ssize_t bytes_read;

        bytes_read = read(
            fd,
            buffer,
            lines[number - 1].length
        );


        if (bytes_read == -1)
        {
            perror("read");
            continue;
        }


        buffer[bytes_read] = '\0';

        printf("%s\n", buffer);
    }


    close(fd);

    return 0;
}