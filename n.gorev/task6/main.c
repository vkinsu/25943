#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <termios.h>

typedef struct {
    off_t offset;
    size_t length;
} LineInfo;

struct termios saved; /* настройки терминала до ввода */
int raw_mode = 0;     /* 1 — терминал сейчас в нашем режиме */

/* Ввод в буфер по одному символу. На экран попадают только допустимые символы,
   стрелки, табы и прочие служебные клавиши пропускаются.
   digits = 1 — разрешены только цифры. Возвращает длину строки или -1 (Ctrl+D, Ctrl+C) */
int read_input(char *buf, int size, int digits) {
    int tty = isatty(0);
    fflush(stdout);

    if (tty) {
        struct termios raw;
        tcgetattr(0, &saved);
        raw = saved;
        raw.c_lflag &= ~(ICANON | ECHO | ISIG); /* без построчного ввода, без эха, без Ctrl+C */
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;
        tcsetattr(0, TCSANOW, &raw);
        raw_mode = 1;
    }

    int len = 0;
    while (1) {
        int c = getchar();

        if (c == EOF || c == 4 || c == 3) { /* конец ввода, Ctrl+D, Ctrl+C */
            len = -1;
            break;
        }

        if (c == '\n' || c == '\r') {
            if (tty) putchar('\n');
            break;
        }

        if (c == 127 || c == '\b') {        /* backspace — стираем символ и с экрана */
            if (len > 0) {
                len--;
                if (tty) printf("\b \b");
            }
            fflush(stdout);
            continue;
        }

        if (c == 27) {                      /* стрелки и т.п.: ESC [ ... буква */
            c = getchar();
            if (c == '[' || c == 'O')
                while ((c = getchar()) != EOF && !(c >= 0x40 && c <= 0x7E))
                    continue;
            continue;
        }

        if (digits ? !isdigit(c) : !isgraph(c))
            continue;                       /* таб, пробел, служебные — не берём */

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

int fd; /* глобальный, чтобы обработчик сигнала мог читать файл */

/* Срабатывает через 5 секунд, если номер не введён: печатает весь файл и завершает программу */
void timeout(int sig) {
    (void)sig;
    char buf[256];
    ssize_t r;

    if (raw_mode)
        tcsetattr(0, TCSANOW, &saved); /* возвращаем терминал в обычный режим */

    write(1, "\nвремя вышло\n", sizeof("\nвремя вышло\n") - 1);
    lseek(fd, 0L, 0);
    while ((r = read(fd, buf, sizeof(buf))) > 0)
        write(1, buf, r);

    _exit(0);
}

int main(void) {
    char filename[256];

    printf("имя файла: ");
    if (read_input(filename, sizeof(filename), 0) <= 0)
        return 1;

    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("ошибка чтения");
        return 1;
    }

    size_t capacity = 16;
    size_t line_count = 0;
    LineInfo *table = malloc(capacity * sizeof(LineInfo));

    off_t line_start = 0;
    char ch;

    while (read(fd, &ch, 1) > 0) {
        if (ch == '\n') {
            off_t pos = lseek(fd, 0L, 1);

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

    off_t file_size = lseek(fd, 0L, 1);
    if (file_size > line_start) {
        if (line_count == capacity) {
            capacity += 1;
            LineInfo *tmp = realloc(table, capacity * sizeof(LineInfo));

            table = tmp;
        }
        table[line_count].offset = line_start;
        table[line_count].length = (size_t)(file_size - line_start);
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

        alarm(5);                 /* запускаем таймер */
        int len = read_input(input, sizeof(input), 1);
        alarm(0);                 /* успели — отменяем таймер */

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
        char *buf = malloc(li->length + 1);

        lseek(fd, li->offset, 0);
        ssize_t r = read(fd, buf, li->length);
        buf[r] = '\0';

        printf("%s", buf);
        if (r == 0 || buf[r - 1] != '\n')
            printf("\n");

        free(buf);
    }

    free(table);
    close(fd);
    return 0;
}