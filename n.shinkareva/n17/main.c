#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

#define MAX_WIDTH 40

static struct termios previous_settings;
static int terminal_is_changed = 0;

static char current_line[MAX_WIDTH];
static size_t line_length = 0;

// возвращение терминалу его исходные настройки
static void restore_terminal(void)
{
    if (terminal_is_changed) {
        tcsetattr(STDIN_FILENO, TCSANOW, &previous_settings);
    }
}

// вывод символов сразу, без ожидания заполнения буфера
static void show(const char *text, size_t length)
{
    while (length > 0) {
        ssize_t count = write(STDOUT_FILENO, text, length);

        if (count < 0 && errno == EINTR) {
            continue;
        }

        if (count <= 0) {
            return;
        }

        text += count;
        length -= (size_t)count;
    }
}

// подача звукового сигнала через символ ctrl-g
static void bell(void)
{
    show("\a", 1);
}

// удаление последнего символа из буфера и с экрана
static void remove_last(void)
{
    if (line_length == 0) {
        bell();
        return;
    }

    line_length--;

    // возвращение курсора, "затирание" символа и снова возвращаюсь
    show("\b \b", 3);
}

// удаление всех символов текущей строки
static void remove_line(void)
{
    while (line_length > 0) {
        remove_last();
    }
}

// удаление последнего слова вместе с пробелами после него
static void remove_word(void)
{
    // сначала стирание пробелов в конце строки
    while (line_length > 0 &&
           current_line[line_length - 1] == ' ') {
        remove_last();
    }

    // затем стирание символов последнего слова
    while (line_length > 0 &&
           current_line[line_length - 1] != ' ') {
        remove_last();
    }
}

// переход на новую строку и очищение ее буфера
static void next_line(void)
{
    show("\r\n", 2);
    line_length = 0;
}

// добавление символа с учетом ограничения в 40 столбцов
static void insert_character(char character)
{
    if (line_length == MAX_WIDTH) {
        size_t beginning = line_length;

        // определение начала последнего слова
        if (character != ' ') {
            while (beginning > 0 &&
                   current_line[beginning - 1] != ' ') {
                beginning--;
            }
        }

        size_t word_length = line_length - beginning;

        if (word_length > 0 && word_length < MAX_WIDTH) {
            char word[MAX_WIDTH];

            // сохранение слова перед переносом
            memcpy(word, current_line + beginning, word_length);

            // убираю слово с предыдущей строки
            while (line_length > beginning) {
                remove_last();
            }

            next_line();

            // перенос слова в начало новой строки
            memcpy(current_line, word, word_length);
            line_length = word_length;
            show(word, word_length);
        } else {
            // слишком длинное слово разделяю на границе
            next_line();
        }
    }

    current_line[line_length++] = character;

    // самостоятельно отображаем введенный символ
    show(&character, 1);
}

int main(void)
{
    struct termios raw_settings;
    unsigned char character;
    int escape_state = 0;
    ssize_t count;

    // проверка, что ввод идет из настоящего терминала
    if (!isatty(STDIN_FILENO) ||
        tcgetattr(STDIN_FILENO, &previous_settings) == -1) {
        fprintf(stderr, "terminal input required\n");
        return 1;
    }

    // сохранение настроек и отключение стандартной обработки ввода
    raw_settings = previous_settings;

    raw_settings.c_lflag &= ~(ICANON | ECHO | ISIG | IEXTEN);
    raw_settings.c_iflag &= ~(IXON | IXOFF);

    // разрешение читать по одному символу без ожидания enter
    raw_settings.c_cc[VMIN] = 1;
    raw_settings.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw_settings) == -1) {
        perror("tcsetattr");
        return 1;
    }

    terminal_is_changed = 1;
    atexit(restore_terminal);

    {
        const char *message =
            "editor: backspace, ctrl-u, ctrl-w, ctrl-d (empty line)\r\n";

        show(message, strlen(message));
    }

    // считывание клавиш и обработка каждой самостоятельно
    while ((count = read(STDIN_FILENO, &character, 1)) != 0) {
        if (count == -1) {
            if (errno == EINTR) {
                continue;
            }

            perror("read");
            break;
        }

        // пропуск escape-последовательности от стрелок
        if (escape_state == 1) {
            escape_state = 0;

            if (character == '[' || character == 'O') {
                escape_state = 2;
                continue;
            }
        } else if (escape_state == 2) {
            if (character >= 0x40 && character <= 0x7e) {
                escape_state = 0;
            }

            continue;
        }

        if (character == 27) {
            // escape и последовательности стрелок
            bell();
            escape_state = 1;
        } else if (
            character == (unsigned char)previous_settings.c_cc[VERASE] ||
            character == 8 || character == 127
        ) {
            // backspace удаляет последний символ
            remove_last();
        } else if (
            character == (unsigned char)previous_settings.c_cc[VKILL]
        ) {
            // kill удаляет всю текущую строку
            remove_line();
        } else if (character == 23) {
            // ctrl-w удаляет последнее слово
            remove_word();
        } else if (character == 4) {
            // ctrl-d завершает работу только в начале строки
            if (line_length == 0) {
                break;
            }

            bell();
        } else if (character == '\r' || character == '\n') {
            // enter начинает новую строку
            next_line();
        } else if (isprint(character)) {
            // обычные символы сразу показываем на экране
            insert_character((char)character);
        } else {
            // остальные управляющие символы вызывают сигнал
            bell();
        }
    }

    // восстанавление настроек перед выходом
    restore_terminal();
    terminal_is_changed = 0;

    show("\r\n", 2);

    return count == -1 ? 1 : 0;
}