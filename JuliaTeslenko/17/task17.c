
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <ctype.h>

#define MAX_LEN 4096
#define WIDTH 40

struct termios old_settings;

void restore_terminal()
{
    tcsetattr(STDIN_FILENO, TCSANOW, &old_settings);
}

void bell()
{
    write(STDOUT_FILENO, "\007", 1);
}

void print_char(char c)
{
    write(STDOUT_FILENO, &c, 1);
}

void redraw(char *buffer, int len)
{
    write(STDOUT_FILENO, "\r\033[J", 4);

    int col = 0;
    int i = 0;

    while (i < len)
    {
        if (buffer[i] == ' ')
        {
            print_char(buffer[i]);
            col++;
            i++;
        }
        else
        {
            int word_len = 0;

            while (i + word_len < len &&
                   buffer[i + word_len] != ' ')
            {
                word_len++;
            }

            if (col > 0 && col + word_len > WIDTH)
            {
                print_char('\n');
                col = 0;
            }

            for (int j = 0; j < word_len; j++)
            {
                if (col == WIDTH)
                {
                    print_char('\n');
                    col = 0;
                }

                print_char(buffer[i + j]);
                col++;
            }

            i += word_len;
        }
    }
}

int main()
{
    struct termios new_settings;

    if (tcgetattr(STDIN_FILENO, &old_settings) == -1)
    {
        perror("tcgetattr");
        return 1;
    }

    new_settings = old_settings;

    new_settings.c_lflag &= ~(ICANON | ECHO | ISIG | IEXTEN);
    new_settings.c_iflag &= ~(IXON | ICRNL | INLCR | IGNCR);

    new_settings.c_cc[VMIN] = 1;
    new_settings.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &new_settings) == -1)
    {
        perror("tcsetattr");
        return 1;
    }

    atexit(restore_terminal);

    char buffer[MAX_LEN];
    int len = 0;
    char c;

    while (1)
    {
        if (read(STDIN_FILENO, &c, 1) != 1)
            break;

        if (c == 27)
{
    char seq[2];

    if (read(STDIN_FILENO, &seq[0], 1) != 1 ||
        read(STDIN_FILENO, &seq[1], 1) != 1)
        continue;

    if (seq[0] == '[')
    {
        if (seq[1] == 'A') // вверх
            bell();
        else if (seq[1] == 'B') // вниз
            bell();
        else if (seq[1] == 'C') // вправо
            bell();
        else if (seq[1] == 'D') // влево
            bell();
    }

    continue;
}

        if (c == old_settings.c_cc[VERASE])
        {
            if (len > 0)
            {
                len--;
                redraw(buffer, len);
            }
        }
        else if (c == old_settings.c_cc[VKILL])
        {
            len = 0;
            redraw(buffer, len);
        }
        else if (c == 23)
        {
            while (len > 0 && buffer[len - 1] == ' ')
                len--;

            while (len > 0 && buffer[len - 1] != ' ')
                len--;

            redraw(buffer, len);
        }
        else if (c == 4)
        {
            if (len == 0)
                break;
            else
                bell();
        }
        else if (isprint((unsigned char)c))
        {
            if (len < MAX_LEN - 1)
            {
                buffer[len++] = c;
                redraw(buffer, len);
            }
            else
            {
                bell();
            }
        }
        else
        {
            bell();
        }
    }

    write(STDOUT_FILENO, "\r\n", 2);

    return 0;
}