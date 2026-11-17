#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

#define MAXLEN 1024

struct node {
    char *str;
    struct node *next;
};

/*
 * Читает одну строку в буфер по одному символу.
 * Терминал переводится в "сырой" режим: он сам ничего не печатает,
 * на экран выводим только то, что приняли в буфер.
 * Стрелки, табы и прочие служебные клавиши пропускаются.
 * Возвращает длину строки (вместе с '\n') или -1 при конце ввода (Ctrl+D, Ctrl+C).
 */
static int read_line(char *buf, int size)
{
    struct termios saved, raw;
    int tty = isatty(0);
    int len = 0;
    int c;

    if (tty) {
        tcgetattr(0, &saved);
        raw = saved;
        raw.c_lflag &= ~(ICANON | ECHO | ISIG); /* без построчного ввода, без эха, без Ctrl+C */
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;
        tcsetattr(0, TCSANOW, &raw);
    }

    while (1) {
        c = getchar();

        if (c == EOF || c == 4 || c == 3) {    /* конец ввода, Ctrl+D, Ctrl+C */
            if (len == 0)
                len = -1;                      /* пустая строка — значит ввод закончен */
            break;
        }

        if (c == '\n' || c == '\r') {          /* Enter — строка готова */
            if (tty)
                putchar('\n');
            buf[len++] = '\n';
            break;
        }

        if (c == 0x7F || c == '\b') {          /* Backspace — стираем символ и с экрана */
            /* русская буква занимает несколько байт: убираем их все */
            while (len > 0 && ((unsigned char)buf[len - 1] & 0xC0) == 0x80)
                len--;
            if (len > 0) {
                len--;
                if (tty)
                    printf("\b \b");
            }
            fflush(stdout);
            continue;
        }

        if (c == 0x1B) {                       /* стрелки и т.п.: ESC [ ... финальный байт */
            c = getchar();
            if (c == '[' || c == 'O')
                while ((c = getchar()) != EOF && !(c >= 0x40 && c <= 0x7E))
                    continue;
            continue;
        }

        if (c < 0x20)                          /* таб и прочие служебные — не берём */
            continue;

        if (len < size - 2) {                  /* место под '\n' и '\0' */
            buf[len++] = (char)c;
            if (tty)
                putchar(c);
            fflush(stdout);
        }
    }

    if (tty)
        tcsetattr(0, TCSANOW, &saved);         /* возвращаем терминал как было */

    if (len >= 0)
        buf[len] = '\0';
    return len;
}

int main(void)
{
    char buf[MAXLEN];
    struct node *head = NULL, *tail = NULL, *p, *next;

    while (read_line(buf, sizeof buf) >= 0) {
        if (buf[0] == '.')
            break;

        size_t len = strlen(buf);

        struct node *n = malloc(sizeof *n);
        if (n == NULL) {
            perror("malloc");
            break;
        }
        n->str = malloc(len + 1);
        if (n->str == NULL) {
            perror("malloc");
            free(n);
            break;
        }
        strcpy(n->str, buf);
        n->next = NULL;

        if (tail == NULL)
            head = n;
        else
            tail->next = n;
        tail = n;
    }

    for (p = head; p != NULL; p = p->next)
        fputs(p->str, stdout);

    for (p = head; p != NULL; p = next) {
        next = p->next;
        free(p->str);
        free(p);
    }
    return 0;
}
