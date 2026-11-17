#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

int main(void)
{
    FILE *fp;

    printf("UID: %d\n", (int)getuid());
    printf("E UID: %d\n", (int)geteuid());

    fp = fopen("file", "r");

    if (fp == NULL) {
        perror("fopen");
    } else {
        printf("Файл успешно открыт\n");
        fclose(fp);
    }

    if (setuid(getuid()) == -1) {
        perror("setuid");
        return 1;
    }

    printf("UID: %d\n", (int)getuid());
    printf("E UID: %d\n", (int)geteuid());

    fp = fopen("file", "r");

    if (fp == NULL) {
        perror("fopen");
    } else {
        printf("Файл успешно открыт\n");
        fclose(fp);
    }

    return 0;
}
