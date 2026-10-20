#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include "input.h"

#define MAX_LINES 100
struct Line { off_t offset; size_t length; };
static volatile sig_atomic_t time_is_up = 0;
static void alarm_handler(int signum) { (void)signum; time_is_up = 1; }

int main(int argc, char **argv)
{
    if (argc != 2) { fprintf(stderr, "Usage: %s filename\n", argv[0]); return 1; }
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) { perror("open"); return 1; }
    struct Line lines[MAX_LINES];
    int count = 0;
    off_t start = 0, position;
    char ch;
    ssize_t nr;
    while ((nr = read(fd, &ch, 1)) == 1) {
        if (ch != '\n') continue;
        position = lseek(fd, 0L, SEEK_CUR);
        if (position == (off_t)-1) { perror("lseek"); close(fd); return 1; }
        if (count == MAX_LINES) { fprintf(stderr, "Too many lines\n"); close(fd); return 1; }
        lines[count].offset = start;
        lines[count++].length = (size_t)(position - start);
        start = position;
    }
    if (nr == -1) { perror("read"); close(fd); return 1; }
    position = lseek(fd, 0L, SEEK_CUR);
    if (position == (off_t)-1) { perror("lseek"); close(fd); return 1; }
    if (position > start) {
        if (count == MAX_LINES) { fprintf(stderr, "Too many lines\n"); close(fd); return 1; }
        lines[count].offset = start;
        lines[count++].length = (size_t)(position - start);
    }
    struct sigaction action = {0};
    action.sa_handler = alarm_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    if (sigaction(SIGALRM, &action, NULL) == -1) { perror("sigaction"); close(fd); return 1; }
    printf("Таблица строк:\n\n№\tОтступ\tДлина\n");
    for (int i = 0; i < count; i++)
        printf("%d\t%ld\t%lu\n", i+1, (long)lines[i].offset, (unsigned long)lines[i].length);
    while (1) {
        int number;
        printf("\nВведите номер строки (0 - выход): "); fflush(stdout);
        time_is_up = 0;
        alarm(5);
        int result = input_number(&number, &time_is_up);
        alarm(0);
        if (result == 2 || time_is_up) break;
        if (result == 0) { close(fd); return 0; }
        if (result == -1) { puts("Некорректный номер строки"); continue; }
        if (number == 0) { close(fd); return 0; }
        if (number > count) { puts("Такой строки нет"); continue; }
        if (lseek(fd, lines[number-1].offset, SEEK_SET) == (off_t)-1) { perror("lseek"); close(fd); return 1; }
        size_t len = lines[number-1].length;
        char *line = malloc(len+1);
        if (!line) { perror("malloc"); close(fd); return 1; }
        size_t done = 0;
        while (done < len) {
            nr = read(fd, line+done, len-done);
            if (nr <= 0) { if (nr < 0) perror("read"); else fputs("Unexpected EOF\n", stderr); free(line); close(fd); return 1; }
            done += (size_t)nr;
        }
        line[len] = '\0';
        printf("%s", line);
        if (len == 0 || line[len-1] != '\n') putchar('\n');
        free(line);
    }
    printf("\nВремя истекло. Содержимое файла:\n\n");
    if (lseek(fd, 0L, SEEK_SET) == (off_t)-1) { perror("lseek"); close(fd); return 1; }
    while ((nr = read(fd, &ch, 1)) > 0) putchar((unsigned char)ch);
    if (nr < 0) { perror("read"); close(fd); return 1; }
    if (close(fd) == -1) { perror("close"); return 1; }
    return 0;
}
