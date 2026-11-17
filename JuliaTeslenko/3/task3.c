#include <stdio.h>
#include <unistd.h>

int main(void)
{
    FILE *file;

    printf("Before setuid:\n");
    printf("Real UID: %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());

    file = fopen("data.txt", "r");

    if (file == NULL)
        perror("fopen");
    else {
        printf("File opened\n");
        fclose(file);
    }

    if (setuid(getuid()) == -1) {
        perror("setuid");
        return 1;
    }

    printf("\nAfter setuid:\n");
    printf("Real UID: %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());

    file = fopen("data.txt", "r");

    if (file == NULL)
        perror("fopen");
    else {
        printf("File opened\n");
        fclose(file);
    }

    return 0;
}