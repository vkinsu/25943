#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <signal.h>

#define MAX_LINES 1000

typedef struct {
    off_t offset;
    off_t length;
} LineInfo;

volatile sig_atomic_t timeout_flag = 0;

void alarm_handler(int sig) {
    timeout_flag = 1;
}

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

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
        perror("fstat");
        close(fd);
        return 1;
    }

    if (sb.st_size == 0) {
        printf("File is empty.\n");
        close(fd);
        return 0;
    }

    char *addr = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    close(fd);

    LineInfo table[MAX_LINES];
    int line_count = 0;
    off_t line_start = 0;

    for (off_t i = 0; i < sb.st_size; i++) {
        if (addr[i] == '\n') {
            table[line_count].offset = line_start;
            table[line_count].length = (i + 1) - line_start;
            line_count++;
            line_start = i + 1;

            if (line_count >= MAX_LINES) {
                break;
            }
        }
    }

    if (line_start < sb.st_size && line_count < MAX_LINES) {
        table[line_count].offset = line_start;
        table[line_count].length = sb.st_size - line_start;
        line_count++;
    }

    printf("Line Table (total: %d)\n", line_count);
    for (int i = 0; i < line_count; i++) {
        printf("Line %d: offset = %ld, length = %ld\n", i + 1, (long)table[i].offset, (long)table[i].length);
    }

    signal(SIGALRM, alarm_handler);

    int first_attempt = 1;
    int line_num;

    while (1) {
        printf("Enter line number (0 to quit): ");
        fflush(stdout);

        if (first_attempt) {
            alarm(5);
        }

        int scan_res = scanf("%d", &line_num);

        if (first_attempt) {
            alarm(0);
            first_attempt = 0;
            if (timeout_flag) {
                printf("\nTimeout reached. Printing entire file:\n");
                fwrite(addr, 1, sb.st_size, stdout);
                munmap(addr, sb.st_size);
                return 0;
            }
        }

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

        off_t offset = table[line_num - 1].offset;
        off_t len = table[line_num - 1].length;

        printf("Line %d: ", line_num);
        fwrite(addr + offset, 1, len, stdout);
        if (addr[offset + len - 1] != '\n') {
            printf("\n");
        }
    }

    munmap(addr, sb.st_size);
    return 0;
}
