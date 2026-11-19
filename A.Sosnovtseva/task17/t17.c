#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <poll.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define LIMIT 40

static void remove_chars(int count)
{
    while (count-- > 0)
        fputs("\b \b", stdout);
}

static int escape_byte(unsigned char *key)
{
    struct pollfd input;
    int ready;

    input.fd = STDIN_FILENO;
    input.events = POLLIN;

    do {
        ready = poll(&input, 1, 100);
    } while (ready == -1 && errno == EINTR);

    return ready > 0 && read(STDIN_FILENO, key, 1) == 1;
}

static int skip_special_key(void)
{
    unsigned char part;

    if (!escape_byte(&part))
        return -1;

    if (part != '[' && part != 'O')
        return part;

    while (escape_byte(&part)) {
        if (part >= 0x40 && part <= 0x7e)
            break;
    }

    return -1;
}

int main(void)
{
    struct termios saved, mode;
    char text[LIMIT];
    unsigned char key;
    int used = 0, cut, tail, next, failed = 0;
    ssize_t n;

    if (tcgetattr(STDIN_FILENO, &saved) == -1) {
        perror("tcgetattr");
        return 1;
    }

    mode = saved;
    mode.c_lflag &= ~(ICANON | ECHO | ECHONL | ISIG | IEXTEN);
    mode.c_iflag &= ~(IXON | IXOFF | ICRNL | INLCR | IGNCR | ISTRIP);
    mode.c_oflag &= ~OPOST;
    mode.c_cc[VMIN] = 1;
    mode.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &mode) == -1) {
        perror("tcsetattr");
        return 1;
    }

    setvbuf(stdout, NULL, _IONBF, 0);

    for (;;) {
        n = read(STDIN_FILENO, &key, 1);

        if (n == -1 && errno == EINTR)
            continue;

        if (n <= 0) {
            failed = (n == -1);
            break;
        }

handle_key:
        if (key == saved.c_cc[VERASE]) {
            if (used > 0) {
                remove_chars(1);
                --used;
            }
        } else if (key == saved.c_cc[VKILL]) {
            remove_chars(used);
            used = 0;
        } else if (key == 23) { //Ctrl-W
            cut = used;

            while (cut > 0 && text[cut - 1] == ' ')
                --cut;

            while (cut > 0 && text[cut - 1] != ' ')
                --cut;

            remove_chars(used - cut);
            used = cut;
        } else if (key == 4) { //Ctrl-D
            if (used == 0)
                break;
        } else if (key == 27) { //Escape
            next = skip_special_key();
            putchar('\a');

            if (next >= 0) {
                key = (unsigned char)next;
                goto handle_key;
            }
        } else if (!isprint(key)) {
            putchar('\a');
        } else {
            if (used == LIMIT) {
                if (key == ' ') {
                    fputs("\r\n", stdout);
                    used = 0;
                    continue;
                }

                cut = used;
                while (cut > 0 && text[cut - 1] != ' ')
                    --cut;

                tail = (cut == 0) ? 0 : used - cut;

                remove_chars(tail);
                fputs("\r\n", stdout);

                memmove(text, text + cut, tail);
                used = tail;
                fwrite(text, 1, used, stdout);
            }

            text[used++] = (char)key;
            putchar(key);
        }
    }

    fputs("\r\n", stdout);

    if (failed)
        perror("read");

    if (tcsetattr(STDIN_FILENO, TCSANOW, &saved) == -1) {
        perror("tcsetattr");
        return 1;
    }

    return failed;
}