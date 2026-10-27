#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#define BLOCK_SIZE 4096
#define WAIT_SECONDS 5

struct line_entry {
    off_t position;
    off_t bytes;
};

static volatile sig_atomic_t time_expired = 0;

static void dump_text(int file_fd)
{
    if (lseek(file_fd, 0, SEEK_SET) == (off_t)-1)
    {
        perror("FATAL ERROR lseek");
        return;
    }
    char block[BLOCK_SIZE];
    ssize_t read_count;
    while ((read_count = read(file_fd, block, sizeof(block))) > 0)
    {
        ssize_t written = 0;
        while (written < read_count)
        {
            ssize_t written_now = write(STDOUT_FILENO, block + written, read_count - written);
            if (written_now < 0)
            {
                perror("ERROR write");
                return;
            }
            written += written_now;
        }
    }
    if (read_count < 0)
        perror("ERROR read");
}

static void alarm_handler(int signal_number)
{
    (void)signal_number;
    time_expired = 1;
}

static int prepare_alarm(void)
{
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = alarm_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    return sigaction(SIGALRM, &action, NULL);
}

int main(int argument_count, char *arguments[])
{
    if (argument_count != 2)
    {
        printf("Using: %s <file>\n", arguments[0]);
        return 1;
    }

    int file_fd = open(arguments[1], O_RDONLY);
    if (file_fd == -1)
    {
        perror("FATAL ERROR opening");
        return 1;
    }

    struct line_entry *line_index = NULL;
    size_t line_count = 0, allocated = 0;
    off_t current_start = 0, total_read = 0;
    char block[BLOCK_SIZE];
    ssize_t read_count;

     
    while ((read_count = read(file_fd, block, sizeof(block))) > 0)
    {
        for (ssize_t k = 0; k < read_count; k++)
        {
            if (block[k] == '\n')
            {
                off_t next_start = total_read + k + 1;
                off_t bytes = next_start - current_start;

                if (line_count == allocated)
                {
                    allocated = allocated ? allocated * 2 : 16;
                    struct line_entry *expanded =
                        realloc(line_index, allocated * sizeof(*line_index));
                    if (!expanded)
                    {
                        perror("FATAL ERROR of realloc");
                        close(file_fd);
                        free(line_index);
                        return 1;
                    }
                    line_index = expanded;
                }
                line_index[line_count].position = current_start;
                line_index[line_count].bytes = bytes;
                line_count++;
                current_start = next_start;
            }
        }
        total_read += read_count;
    }
    if (read_count < 0)
    {
        perror("FATAL ERROR of read");
        close(file_fd);
        free(line_index);
        return 1;
    }
    if (total_read > current_start)
    {
        if (line_count == allocated)
        {
            allocated = allocated ? allocated * 2 : 16;
            struct line_entry *expanded = realloc(line_index, allocated * sizeof(*line_index));
            if (!expanded)
            {
                perror("FATAL ERROR of realloc");
                close(file_fd);
                free(line_index);
                return 1;
            }
            line_index = expanded;
        }
        line_index[line_count].position = current_start;
        line_index[line_count].bytes = total_read - current_start;
        line_count++;
    }

    printf("Table of strings (at all: %zu):\n", line_count);
    for (size_t k = 0; k < line_count; k++)
        printf("  string %2zu: offset = %lld, length = %lld\n",
               k, (long long)line_index[k].position, (long long)line_index[k].bytes);

    if (prepare_alarm() == -1)
    {
        perror("FATAL ERROR of sigaction");
        close(file_fd);
        free(line_index);
        return 1;
    }

    int waiting_first = 1;
    char request[64];

    while (1)
    {
        printf("\nWrite the number of string (0 - exit): ");
        fflush(stdout);

        if (waiting_first)
        {
            errno = 0;
            time_expired = 0;
            alarm(WAIT_SECONDS);
        }

        if (!fgets(request, sizeof(request), stdin))
        {
            if (waiting_first)
                alarm(0);

            if (waiting_first && errno == EINTR && time_expired)
            {
                printf("\nTime is up! Printing whole file:\n");
                dump_text(file_fd);
                printf("\n");
                break;
            }
            if (feof(stdin))
            {
                printf("\nEOF - exit\n");
                break;
            }
            perror("FATAL ERROR of fgets");
            break;
        }

        if (waiting_first)
        {
            alarm(0);
            waiting_first = 0;
        }

        char *parse_end;
        long line_number = strtol(request, &parse_end, 10);

        if (parse_end == request)
        {
            printf("ERROR, enter an integer!\n");
            continue;
        }
        while (*parse_end == ' ' || *parse_end == '\t') parse_end++;
        if (*parse_end != '\n' && *parse_end != '\0')
        {
            printf("ERROR, enter an integer!\n");
            continue;
        }

        if (line_number == 0) break;
        if (line_number < 1 || (size_t)line_number > line_count)
        {
            printf("There is no string like that. allowed: 1..%zu\n", line_count);
            continue;
        }

        size_t selected_index = (size_t)line_number - 1;
        struct line_entry selected = line_index[selected_index];

         

        if (lseek(file_fd, selected.position, SEEK_SET) == (off_t)-1)
        {
            perror("ERROR of lseek");
            continue;
        }

         
       
        char *line_text = malloc(selected.bytes + 1);
        if (!line_text)
        {
            perror("ERROR of malloc");
            continue;
        }

         

        ssize_t received = read(file_fd, line_text, selected.bytes);
        if (received < 0)
        {
            perror("ERROR of read");
            free(line_text);
            continue;
        }

        line_text[received] = '\0';
        char label[64];
        int label_size = snprintf(label, sizeof(label), "String %ld: ", line_number);
        write(STDOUT_FILENO, label, label_size);
        write(STDOUT_FILENO, line_text, received);
        if (received == 0 || line_text[received - 1] != '\n')
            write(STDOUT_FILENO, "\n", 1);

         
        free(line_text);
    }

     

    free(line_index);
    close(file_fd);
    return 0;
}
