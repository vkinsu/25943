#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h>
#include <sys/mman.h>
#include <sys/stat.h>

typedef struct {
    off_t offset;
    size_t length;
} LineInfo;

struct termios saved;
int raw_mode = 0;

int read_input(char *buf, int size, int digits) {
    int tty = isatty(0);
    fflush(stdout);

    if (tty) {
        struct termios raw;
        tcgetattr(0, &saved);
        raw = saved;
        raw.c_lflag &= ~(ICANON | ECHO | ISIG);
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;
        tcsetattr(0, TCSANOW, &raw);
        raw_mode = 1;
    }

    int len = 0;
    while (1) {
        int c = getchar();

        if (c == EOF || c == 4 || c == 3) {
            len = -1;
            break;
        }

        if (c == '\n' || c == '\r') {
            if (tty) putchar('\n');
            break;
        }

        if (c == 127 || c == '\b') {
            while (len > 0 && ((unsigned char)buf[len - 1] & 0xC0) == 0x80)
                len--;
            if (len > 0) {
                len--;
                if (tty) printf("\b \b");
            }
            fflush(stdout);
            continue;
        }

        if (c == 27) {
            c = getchar();
            if (c == '[' || c == 'O')
                while ((c = getchar()) != EOF && !(c >= 0x40 && c <= 0x7E))
                    continue;
            continue;
        }

        if (digits ? !isdigit(c) : (c < 0x80 && !isgraph(c)))
            continue;

        if (len < size - 1) {
            buf[len++] = (char)c;
            if (tty) putchar(c);
            fflush(stdout);
        }
    }

    if (raw_mode) {
        tcsetattr(0, TCSANOW, &saved);
        raw_mode = 0;
    }

    if (len >= 0)
        buf[len] = '\0';
    return len;
}

char *map;
off_t size;

void timeout(int sig) {
    (void)sig;

    if (raw_mode)
        tcsetattr(0, TCSANOW, &saved);

    printf("\nвремя вышло\n%.*s\n", (int)size, map);
    exit(0);
}

int main(void) {
    char filename[256];

    int name_len;
    do {
        printf("имя файла: ");
        name_len = read_input(filename, sizeof(filename), 0);
        if (name_len < 0)
            return 1;
    } while (name_len == 0);

    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("ошибка чтения");
        return 1;
    }

    struct stat st;
    fstat(fd, &st);
    size = st.st_size;
    if (size == 0) {
        printf("файл пустой\n");
        return 0;
    }

    map = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map == MAP_FAILED) {
        perror("ошибка mmap");
        return 1;
    }
    close(fd);

    size_t capacity = 16;
    size_t line_count = 0;
    LineInfo *table = malloc(capacity * sizeof(LineInfo));

    off_t line_start = 0;

    for (off_t i = 0; i < size; i++) {
        if (map[i] == '\n') {
            off_t pos = i + 1;

            if (line_count == capacity) {
                capacity *= 2;
                LineInfo *tmp = realloc(table, capacity * sizeof(LineInfo));

                table = tmp;
            }

            table[line_count].offset = line_start;
            table[line_count].length = (size_t)(pos - line_start);
            line_count++;

            line_start = pos;
        }
    }

    if (size > line_start) {
        if (line_count == capacity) {
            capacity += 1;
            LineInfo *tmp = realloc(table, capacity * sizeof(LineInfo));

            table = tmp;
        }
        table[line_count].offset = line_start;
        table[line_count].length = (size_t)(size - line_start);
        line_count++;
    }

    printf("\n%-8s %-8s %-8s\n", "строка", "отступ", "длина");
    for (size_t i = 0; i < line_count; i++)
        printf("%-8zu %-8lld %-8zu\n", i + 1, (long long)table[i].offset, table[i].length);
    printf("\n");

    signal(SIGALRM, timeout);

    char input[10];
    while (1) {
        printf("введите номер строки (5 секунд): ");

        alarm(5);
        int len = read_input(input, sizeof(input), 1);
        alarm(0);

        if (len < 0)
            break;

        if (len == 0) {
            printf("плохой ввод\n");
            continue;
        }

        int n = atoi(input);

        if (n == 0)
            break;

        if ((size_t)n > line_count) {
            printf("нет такой строки\n");
            continue;
        }

        LineInfo *li = &table[n - 1];

        printf("%.*s", (int)li->length, map + li->offset);
        if (map[li->offset + li->length - 1] != '\n')
            printf("\n");
    }

    free(table);
    munmap(map, size);
    return 0;
}