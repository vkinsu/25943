
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

// печатаем реальный и эффективный идентификаторы пользователя
void print_user_ids(void)
{
    printf("real uid = %ld\n", (long)getuid());
    printf("effective uid = %ld\n", (long)geteuid());
}

// пытаемся открыть файл для чтения и записи
void check_file_access(void)
{
    FILE *data_file = fopen("access_test.txt", "r+");

    if (data_file == NULL) {
        perror("fopen");
        return;
    }

    printf("file opened successfully\n");

    // закрываем файл, если удалось его открыть
    if (fclose(data_file) == EOF) {
        perror("fclose");
    }
}

int main(void)
{
    // сначала показываем uid и проверяем доступ
    printf("before setuid:\n");
    print_user_ids();
    check_file_access();

    // устанавливаем эффективный uid равным реальному
    if (setuid(getuid()) == -1) {
        perror("setuid");
        return 1;
    }

    // повторяем проверку после изменения uid
    printf("\nafter setuid:\n");
    print_user_ids();
    check_file_access();

    return 0;
}
