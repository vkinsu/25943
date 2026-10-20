#ifndef INPUT_H
#define INPUT_H
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>

/* Returns 1 for valid number, 0 for EOF, -1 for invalid input, 2 for alarm. */
static int input_number(int *value, volatile sig_atomic_t *expired)
{
    char digits[32];
    size_t n = 0;
    int invalid = 0;
    int state = 0;
    int c;

    while (1) {
        if (expired && *expired) return 2;
        errno = 0;
        c = getchar();
        if (c == EOF) {
            if (expired && *expired) return 2;
            if (ferror(stdin)) {
                if (errno == EINTR) {
                    clearerr(stdin);
                    continue;
                }
                perror("getchar");
                return -1;
            }
            if (n == 0 && !invalid && state == 0) return 0;
            break;
        }
        if (c == '\n') break;
        if (state == 1) {
            if (c == '[') { state = 2; continue; }
            invalid = 1;
            state = 0;
        } else if (state == 2) {
            if (c >= 'A' && c <= 'D') { state = 0; continue; }
            invalid = 1;
            state = 0;
        } else if (c == 27) {
            state = 1;
            continue;
        }
        if (c == ' ' || c == '\t') {
            if (n > 0) invalid = 1;
            continue;
        }
        if (c >= '0' && c <= '9' && n < sizeof(digits) - 1) {
            digits[n++] = (char)c;
        } else invalid = 1;
    }
    if (expired && *expired) return 2;
    if (state != 0 || invalid || n == 0) return -1;
    digits[n] = '\0';
    errno = 0;
    char *end;
    long num = strtol(digits, &end, 10);
    if (errno == ERANGE || *end || num > INT_MAX) return -1;
    *value = (int)num;
    return 1;
}
#endif
