#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

#define MAX_LINES 1000

typedef struct {
    off_t offset;
    off_t length;
} LineInfo;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    LineInfo table[MAX_LINES];
    int line_count = 0;
    off_t line_start = 0;
    char ch;

    while (read(fd, &ch, 1) > 0) {
        if (ch == '\n') {
            off_t current_pos = lseek(fd, 0L, 1);
            table[line_count].offset = line_start;
            table[line_count].length = current_pos - line_start;
            line_count++;
            line_start = current_pos;

            if (line_count >= MAX_LINES) {
                break;
            }
        }
    }

    off_t total_size = lseek(fd, 0L, 1);
    if (total_size > line_start && line_count < MAX_LINES) {
        table[line_count].offset = line_start;
        table[line_count].length = total_size - line_start;
        line_count++;
    }

    printf("Line Table (total: %d)\n", line_count);
    for (int i = 0; i < line_count; i++) {
        printf("Line %d: offset = %ld, length = %ld\n", i + 1, (long)table[i].offset, (long)table[i].length);
    }

    int line_num;
    while (1) {
        printf("Enter line number (0 to quit): ");
        int scan_res = scanf("%d", &line_num);

        if (scan_res != 1) {
            printf("Invalid input. Please enter a number.\n");
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            continue;
        }

        if (line_num == 0) {
            break;
        }

        if (line_num < 1 || line_num > line_count) {
            printf("Invalid line number. Total lines: %d\n", line_count);
            continue;
        }

        off_t len = table[line_num - 1].length;
        lseek(fd, table[line_num - 1].offset, SEEK_SET);

        char *buf = (char *)malloc(len + 1);
        if (buf == NULL) {
            perror("malloc");
            close(fd);
            return 1;
        }

        ssize_t n = read(fd, buf, len);
        if (n > 0) {
            buf[n] = '\0';
            printf("Line %d: %s", line_num, buf);
            if (buf[n - 1] != '\n') {
                printf("\n");
            }
        }

        free(buf);
    }

    close(fd);
    return 0;
}
