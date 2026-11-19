#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

// преобразование типа файла и его права доступа в понятную строку
static void make_permissions(mode_t mode, char result[11])
{
    // права на чтение, запись и выполнение для владельца, группы и остальных
    const mode_t bits[9] = {
        S_IRUSR, S_IWUSR, S_IXUSR,
        S_IRGRP, S_IWGRP, S_IXGRP,
        S_IROTH, S_IWOTH, S_IXOTH
    };

    const char symbols[] = "rwxrwxrwx";
    int i;

    // первым символом обозначаем тип файла
    if (S_ISDIR(mode)) {
        result[0] = 'd';
    } else if (S_ISREG(mode)) {
        result[0] = '-';
    } else {
        result[0] = '?';
    }

    // проверка каждого бита прав доступа
    // если право есть, записываем r, w или x, иначе -
    for (i = 0; i < 9; i++) {
        result[i + 1] = (mode & bits[i]) ? symbols[i] : '-';
    }

    // завершение строки нулевым символом
    result[10] = '\0';
}

// в выводе только имя файла, без пути к нему
static void print_short_name(const char *path)
{
    const char *end = path + strlen(path);
    const char *begin;

    // убираю завершающие слеши, если они есть
    while (end > path && end[-1] == '/') {
        end--;
    }

    // отдельная обработка корневой директории
    if (end == path) {
        printf("/");
        return;
    }

    // поиск начала имени после последнего слеша
    begin = end;

    while (begin > path && begin[-1] != '/') {
        begin--;
    }

    printf("%.*s", (int)(end - begin), begin);
}

int main(int argc, char *argv[])
{
    int i;
    int errors = 0;

    // проверка, передал ли пользователь имена файлов или каталогов
    if (argc < 2) {
        fprintf(stderr, "usage: %s file...\n", argv[0]);
        return 1;
    }

    // вывод заголовков столбцов
    printf("%-10s %5s %-15s %-15s %10s %-16s %s\n",
           "permissions", "links", "owner", "group",
           "size", "modified", "name");

    // последовательная обработка каждого аргумента командной строки
    for (i = 1; i < argc; i++) {
        struct stat info;
        struct passwd *owner;
        struct group *group;
        struct tm *time_info;

        char permissions[11];
        char links[32] = "";
        char size[32] = "";
        char date[32];
        char owner_number[32];
        char group_number[32];

        const char *owner_name;
        const char *group_name;

        // получение информации о файле через lstat
        // при ошибке сообщаем о ней и переходим к следующему файлу
        if (lstat(argv[i], &info) == -1) {
            perror(argv[i]);
            errors = 1;
            continue;
        }

        // определение типа файла и прав доступа
        make_permissions(info.st_mode, permissions);

        // получение имени владельца по его числовому идентификатору uid
        owner = getpwuid(info.st_uid);

        if (owner != NULL) {
            owner_name = owner->pw_name;
        } else {
            // если имя не найдено, вывод числового uid
            snprintf(owner_number, sizeof(owner_number), "%lu",
                     (unsigned long)info.st_uid);
            owner_name = owner_number;
        }

        // получение имени группы по числовому идентификатору gid
        group = getgrgid(info.st_gid);

        if (group != NULL) {
            group_name = group->gr_name;
        } else {
            // если имя группы не найдено, вывод числового gid
            snprintf(group_number, sizeof(group_number), "%lu",
                     (unsigned long)info.st_gid);
            group_name = group_number;
        }

        // по вашему требованию вывожу ссылки только для каталогов
        // для обычных файлов поле links остается пустым
        if (S_ISDIR(info.st_mode)) {
            snprintf(links, sizeof(links), "%lu",
                     (unsigned long)info.st_nlink);
        }

        // размер в байтах выводится только для обычных файлов
        // для каталогов и других типов файлов поле size остается пустым
        if (S_ISREG(info.st_mode)) {
            snprintf(size, sizeof(size), "%lld",
                     (long long)info.st_size);
        }

        // преобразование времени последнего изменения файла в обычную дату
        time_info = localtime(&info.st_mtime);

        if (time_info == NULL ||
            strftime(date, sizeof(date), "%d.%m.%Y %H:%M",
                     time_info) == 0) {
            strcpy(date, "?");
        }

        // вывод информации о файле в виде строки таблицы
        printf("%-10s %5s %-15s %-15s %10s %-16s ",
               permissions,
               links,
               owner_name,
               group_name,
               size,
               date);

        // вывожу только имя файла без полного пути
        print_short_name(argv[i]);
        putchar('\n');
    }

    // возвращаем код ошибки, если какой-то файл обработать не удалось
    return errors ? 1 : 0;
}