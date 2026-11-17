#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <termios.h>

#define BYTE_SIZE 1

struct line_entry {
    off_t position;
    off_t bytes;
};

static struct termios previous_terminal;
static int terminal_saved = 0;

static void clean_request(char *text)
{
    size_t source_index = 0, destination_index = 0;
    while (text[source_index] != '\0')
    {
        unsigned char byte = (unsigned char)text[source_index];
        if (byte == '\n' || byte == '\t' || (byte >= 0x20 && byte < 0x7F))
            text[destination_index++] = text[source_index];
        source_index++;
    }
    text[destination_index] = '\0';
}

static void reset_terminal(void)
{
    if (terminal_saved)
    {
        tcsetattr(STDIN_FILENO, TCSANOW, &previous_terminal);
        terminal_saved = 0;
    }
}

static void prepare_terminal(void)
{
    struct termios settings;
    if (tcgetattr(STDIN_FILENO, &settings) == -1)
        return;
    previous_terminal = settings;
    terminal_saved = 1;
    settings.c_lflag |= ICANON | ECHO | ISIG | IEXTEN;
    settings.c_lflag &= ~ECHOCTL;
    tcsetattr(STDIN_FILENO, TCSANOW, &settings);
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
    off_t current_start = 0;
    char block[BYTE_SIZE];
    ssize_t read_count;

    while ((read_count = read(file_fd, block, BYTE_SIZE)) > 0)
    {
        off_t total_read = lseek(file_fd, 0L, SEEK_CUR);
        if (block[0] == '\n')
        {
            off_t next_start = total_read;
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
    if (read_count < 0)
    {
        perror("FATAL ERROR of read");
        close(file_fd);
        free(line_index);
        return 1;
    }

    off_t total_size = lseek(file_fd, 0L, SEEK_CUR);
    if (total_size > current_start)
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
        line_index[line_count].bytes = total_size - current_start;
        line_count++;
    }

    printf("Table of strings (at all: %zu):\n", line_count);
    for (size_t k = 0; k < line_count; k++)
        printf("  string %2zu: offset = %lld, length = %lld\n",
               k, (long long)line_index[k].position, (long long)line_index[k].bytes);

    prepare_terminal();

    char request[64];
    while (1)
    {
        printf("\nWrite the number of string (0 - exit): ");
        fflush(stdout);

        memset(request, 0, sizeof(request));

        if (!fgets(request, sizeof(request), stdin))
        {
            if (feof(stdin))
                printf("\nEOF - exit\n");
            else
                perror("FATAL ERROR of fgets");
            break;
        }

        clean_request(request);

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

    reset_terminal();
    free(line_index);
    close(file_fd);
    return 0;
}
