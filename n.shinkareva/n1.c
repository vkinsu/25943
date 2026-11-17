
#define _XOPEN_SOURCE 600

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <ulimit.h>
#include <errno.h>

extern char **environ;

// структура для хранения опции и ее аргумента
struct SavedOption {
    int name;
    char *argument;
    int error_name;
};

// проверяем, что строка содержит правильное неотрицательное число
int read_number(const char *text, long *number)
{
    char *end;

    errno = 0;
    *number = strtol(text, &end, 10);

    return text[0] != '\0' &&
           *end == '\0' &&
           errno == 0 &&
           *number >= 0;
}

// выполняем действие одной опции
int run_option(struct SavedOption current_option)
{
    struct rlimit core_limit;
    long number;
    long file_limit;
    char directory[4096];
    char **variable;
    char *equal_sign;

    switch (current_option.name) {

        case 'i':
            // выводим реальные и эффективные идентификаторы
            printf("uid = %ld, euid = %ld\n",
                   (long)getuid(), (long)geteuid());

            printf("gid = %ld, egid = %ld\n",
                   (long)getgid(), (long)getegid());
            break;

        case 's':
            // делаем процесс лидером группы
            if (setpgid(0, 0) == -1) {
                perror("setpgid");
                return 1;
            }

            printf("new pgid = %ld\n", (long)getpgrp());
            break;

        case 'p':
            // выводим идентификаторы процесса, родителя и группы
            printf("pid = %ld\n", (long)getpid());
            printf("ppid = %ld\n", (long)getppid());
            printf("pgid = %ld\n", (long)getpgrp());
            break;

        case 'u':
            // получаем ulimit в блоках по 512 байт
            file_limit = ulimit(UL_GETFSIZE);

            if (file_limit == -1) {
                perror("ulimit");
                return 1;
            }

            printf("ulimit = %ld\n", file_limit);
            break;

        case 'U':
            // проверяем новое значение ulimit
            if (!read_number(current_option.argument, &number)) {
                printf("invalid value for -U: %s\n",
                       current_option.argument);
                return 1;
            }

            // устанавливаем новый ulimit
            if (ulimit(UL_SETFSIZE, number) == -1) {
                perror("ulimit");
                return 1;
            }

            printf("new ulimit = %ld\n", number);
            break;

        case 'c':
            // получаем ограничение размера core-файла
            if (getrlimit(RLIMIT_CORE, &core_limit) == -1) {
                perror("getrlimit");
                return 1;
            }

            if (core_limit.rlim_cur == RLIM_INFINITY) {
                printf("core size = unlimited\n");
            }
            else {
                printf("core size = %llu bytes\n",
                       (unsigned long long)core_limit.rlim_cur);
            }
            break;

        case 'C':
            // проверяем новый размер core-файла
            if (!read_number(current_option.argument, &number)) {
                printf("invalid value for -C: %s\n",
                       current_option.argument);
                return 1;
            }

            // получаем текущие ограничения
            if (getrlimit(RLIMIT_CORE, &core_limit) == -1) {
                perror("getrlimit");
                return 1;
            }

            // изменяем мягкое ограничение размера core-файла
            core_limit.rlim_cur = (rlim_t)number;

            if (setrlimit(RLIMIT_CORE, &core_limit) == -1) {
                perror("setrlimit");
                return 1;
            }

            printf("new core size = %ld bytes\n", number);
            break;

        case 'd':
            // получаем текущую рабочую директорию
            if (getcwd(directory, sizeof(directory)) == NULL) {
                perror("getcwd");
                return 1;
            }

            printf("directory = %s\n", directory);
            break;

        case 'v':
            // выводим все переменные окружения
            for (variable = environ;
                 *variable != NULL;
                 variable++) {

                printf("%s\n", *variable);
            }
            break;

        case 'V':
            // проверяем формат name=value
            equal_sign = strchr(current_option.argument, '=');

            if (equal_sign == NULL ||
                equal_sign == current_option.argument) {

                printf("invalid format for -V\n");
                return 1;
            }

            // добавляем или изменяем переменную окружения
            if (putenv(current_option.argument) != 0) {
                perror("putenv");
                return 1;
            }

            printf("variable set: %s\n",
                   current_option.argument);
            break;

        case '?':
            // обрабатываем неизвестную опцию
            printf("unknown option: -%c\n",
                   current_option.error_name);
            return 1;

        case ':':
            // обрабатываем отсутствие значения у опции
            printf("missing argument for -%c\n",
                   current_option.error_name);
            return 1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    struct SavedOption *saved_options = NULL;
    struct SavedOption *new_options;

    int option;
    int option_count = 0;
    int result = 0;
    int i;

    // проверяем, переданы ли аргументы
    if (argc == 1) {
        printf("no options provided\n");
        return 1;
    }

    // отключаем стандартные сообщения об ошибках getopt
    opterr = 0;

    // читаем и сохраняем опции в порядке их появления
    while ((option = getopt(argc, argv,
                            ":ispuU:cC:dvV:")) != -1) {

        // увеличиваем массив для новой опции
        new_options = realloc(
            saved_options,
            (option_count + 1) * sizeof(*saved_options)
        );

        if (new_options == NULL) {
            perror("realloc");
            free(saved_options);
            return 1;
        }

        saved_options = new_options;

        // запоминаем опцию и ее значение
        saved_options[option_count].name = option;
        saved_options[option_count].argument = optarg;
        saved_options[option_count].error_name = optopt;

        option_count++;
    }

    // выполняем опции справа налево по условию задания
    for (i = option_count - 1; i >= 0; i--) {

        if (run_option(saved_options[i]) != 0) {
            result = 1;
        }
    }

    // проверяем наличие лишних аргументов
    for (i = optind; i < argc; i++) {
        printf("unexpected argument: %s\n", argv[i]);
        result = 1;
    }

    // освобождаем выделенную память
    free(saved_options);

    return result;
}
