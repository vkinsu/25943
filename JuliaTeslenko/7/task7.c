#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define MAX_LINES 1000

typedef struct {
    off_t offset;
    off_t length;
} Line;

int fd;
char *file_data;
off_t file_size;
void handle_sigint(int sig)
{
    (void)sig;

    printf("\nCtrl+C pressed. Program continues.\n");
}

void handle_sigalrm(int sig)
{
    (void)sig;

    printf("\nTime is up!\n");
    printf("File contents:\n");
    printf("%.*s", (int)file_size, file_data);

    munmap(file_data, file_size);
    close(fd);

    exit(0);
}


int main(int argc, char *argv[])
{
    Line lines[MAX_LINES];

    int cnt = 0;

    off_t line_start = 0;
    int first_request = 1;


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
    struct stat st;

    if (fstat(fd, &st) == -1)
    {
        perror("fstat");
        close(fd);
        return 1;
    }

    file_size = st.st_size;

    if (file_size == 0)
    {
        printf("File is empty.\n");
        close(fd);
        return 0;
    }

    file_data = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);


    if (file_data == MAP_FAILED)
    {
        perror("mmap");
        close(fd);
        return 1;
    }

    for (off_t i = 0; i < file_size; i++)
    {
        if (file_data[i] == '\n')
        {
            lines[cnt].offset = line_start;
            lines[cnt].length = i - line_start;

            cnt++;
            line_start = i + 1;

            if (cnt >= MAX_LINES)
            {
                break;
            }
        }
    }

    if (line_start < file_size && cnt < MAX_LINES)
    {
        lines[cnt].offset = line_start;
        lines[cnt].length = file_size - line_start;

        cnt++;
    }

    printf("\nLine table:\n");
    printf("Line\tOffset\tLength\n");

    for (int i = 0; i < cnt; i++)
    {
        printf(
            "%d\t%lld\t%lld\n",
            i + 1,
            (long long)lines[i].offset,
            (long long)lines[i].length
        );
    }

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

        printf("%.*s\n", (int)lines[number - 1].length, file_data + lines[number - 1].offset);
    }

    munmap(file_data, file_size);

    close(fd);
    return 0;
}