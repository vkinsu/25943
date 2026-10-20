#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#define WAIT_SECONDS 5

struct line_entry {
    off_t position;
    off_t bytes;
};

static volatile sig_atomic_t time_expired = 0;

static void dump_text(const char *mapping, off_t total_size)
{
    if (!mapping || total_size <= 0)
        return;

    off_t written = 0;
    while (written < total_size)
    {
        ssize_t written_now = write(STDOUT_FILENO, mapping + written, total_size - written);
        if (written_now < 0)
        {
            perror("ERROR write");
            return;
        }
        written += written_now;
    }
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

     
    struct stat metadata;
    if (fstat(file_fd, &metadata) == -1)
    {
        perror("FATAL ERROR of fstat");
        close(file_fd);
        return 1;
    }
    if (!S_ISREG(metadata.st_mode))
    {
        printf("FATAL ERROR: not a regular file\n");
        close(file_fd);
        return 1;
    }

    off_t text_size = metadata.st_size;

    const char *mapping = NULL;
    if (text_size > 0)
    {
         
        
        void *region = mmap(NULL, text_size, PROT_READ, MAP_PRIVATE, file_fd, 0);
        if (region == MAP_FAILED)
        {
            perror("FATAL ERROR of mmap");
            close(file_fd);
            return 1;
        }
        mapping = (const char *)region;
    }

    struct line_entry *line_index = NULL;
    size_t line_count = 0, allocated = 0;
    off_t current_start = 0;

     

    for (off_t k = 0; k < text_size; k++)
    {
        if (mapping[k] == '\n')
        {
            off_t next_start = k + 1;
            off_t bytes = next_start - current_start;

            if (line_count == allocated)
            {
                allocated = allocated ? allocated * 2 : 16;
                struct line_entry *expanded =
                    realloc(line_index, allocated * sizeof(*line_index));
                if (!expanded)
                {
                    perror("FATAL ERROR of realloc");
                    munmap((void *)mapping, text_size);
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

    if (text_size > current_start)
    {
        if (line_count == allocated)
        {
            allocated = allocated ? allocated * 2 : 16;
            struct line_entry *expanded = realloc(line_index, allocated * sizeof(*line_index));
            if (!expanded)
            {
                perror("FATAL ERROR of realloc");
                munmap((void *)mapping, text_size);
                close(file_fd);
                free(line_index);
                return 1;
            }
            line_index = expanded;
        }
        line_index[line_count].position = current_start;
        line_index[line_count].bytes = text_size - current_start;
        line_count++;
    }

    printf("line offset length\n");
    for (size_t k = 0; k < line_count; k++)
        printf("%zu %lld %lld\n",
               k + 1, (long long)line_index[k].position, (long long)line_index[k].bytes);

    if (prepare_alarm() == -1)
    {
        perror("FATAL ERROR of sigaction");
        munmap((void *)mapping, text_size);
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
                dump_text(mapping, text_size);
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

        char label[64];
        int label_size = snprintf(label, sizeof(label), "String %ld: ", line_number);
        write(STDOUT_FILENO, label, label_size);

          

        off_t written = 0;
        while (written < selected.bytes)
        {
            ssize_t written_now = write(STDOUT_FILENO, mapping + selected.position + written,
                              selected.bytes - written);
            if (written_now < 0)
            {
                perror("ERROR write");
                break;
            }
            written += written_now;
        }

        if (selected.bytes == 0 || mapping[selected.position + selected.bytes - 1] != '\n')
            write(STDOUT_FILENO, "\n", 1);
    }

     
    if (mapping)
        munmap((void *)mapping, text_size);

    free(line_index);
    close(file_fd);
    return 0;
}
