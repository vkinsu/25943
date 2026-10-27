#include <stdio.h>
#include <unistd.h>   // getuid, geteuid, setuid
#include <errno.h>

void print_ids_and_try_open(const char *label)
{
    printf("%s\n", label);
    printf("real uid = %d, effective uid = %d\n", getuid(), geteuid());

    FILE *f = fopen("file", "r");
    if (f == NULL) 
    {
        perror("fopen");
    } 
    else 
    {
        printf("файл успешно открыт\n");
        fclose(f);
    }
    printf("\n");
}

int main(void)
{
    // печатаем ID и пробуем открыть файл
    print_ids_and_try_open("до setuid:");

    // делаем реальный и эффективный ID одинаковыми
    if (setuid(getuid()) == -1) 
    {
        perror("setuid");
        return 1;
    }

    // повторяем то же самое
    print_ids_and_try_open("после setuid:");

    return 0;
}