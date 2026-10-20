#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <signal.h>
#include "input.h"

#define MAX_LINES 100
struct Line { size_t offset; size_t length; };
static volatile sig_atomic_t time_is_up = 0;
static void alarm_handler(int signum) { (void)signum; time_is_up = 1; }

int main(int argc, char **argv)
{
    if (argc != 2) { fprintf(stderr, "Usage: %s filename\n", argv[0]); return 1; }
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) { perror("open"); return 1; }
    struct stat st;
    if (fstat(fd, &st) == -1 || st.st_size < 0) { perror("fstat"); close(fd); return 1; }
    size_t file_size = (size_t)st.st_size;
    char *data = NULL;
    if (file_size) {
        data = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (data == MAP_FAILED) { perror("mmap"); close(fd); return 1; }
    }
    close(fd);
    struct Line lines[MAX_LINES];
    int count = 0;
    size_t start = 0;
    for (size_t i = 0; i < file_size; i++) {
        if (data[i] != '\n') continue;
        if (count == MAX_LINES) { fprintf(stderr, "Too many lines\n"); goto fail; }
        lines[count].offset = start;
        lines[count++].length = i + 1 - start;
        start = i + 1;
    }
    if (start < file_size) {
        if (count == MAX_LINES) { fprintf(stderr, "Too many lines\n"); goto fail; }
        lines[count].offset = start;
        lines[count++].length = file_size - start;
    }
    struct sigaction action = {0};
    action.sa_handler = alarm_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    if (sigaction(SIGALRM, &action, NULL) == -1) { perror("sigaction"); goto fail; }
    printf("Таблица строк:\n\n№\tОтступ\tДлина\n");
    for (int i = 0; i < count; i++)
        printf("%d\t%lu\t%lu\n", i+1, (unsigned long)lines[i].offset, (unsigned long)lines[i].length);
    while (1) {
        int number;
        printf("\nВведите номер строки (0 - выход): "); fflush(stdout);
        time_is_up = 0;
        alarm(5);
        int result = input_number(&number, &time_is_up);
        alarm(0);
        if (result == 2 || time_is_up) {
            printf("\nВремя истекло. Содержимое файла:\n\n");
            if (file_size && fwrite(data, 1, file_size, stdout) != file_size) { perror("fwrite"); goto fail; }
            break;
        }
        if (result == 0) break;
        if (result == -1) { puts("Некорректный номер строки"); continue; }
        if (number == 0) break;
        if (number > count) { puts("Такой строки нет"); continue; }
        size_t offset = lines[number-1].offset;
        size_t len = lines[number-1].length;
        if (fwrite(data + offset, 1, len, stdout) != len) { perror("fwrite"); goto fail; }
        if (len == 0 || data[offset + len - 1] != '\n') putchar('\n');
    }
    if (file_size && munmap(data, file_size) == -1) { perror("munmap"); return 1; }
    return 0;
fail:
    if (file_size) munmap(data, file_size);
    return 1;
}
