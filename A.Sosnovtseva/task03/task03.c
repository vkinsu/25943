#include <stdio.h>
#include <unistd.h>

int main(void)
{
    FILE *file;

    printf("Real UID: %ld\n", (long)getuid());
    printf("Effective UID: %ld\n", (long)geteuid());

    file = fopen("data.txt", "r");

    if (file == NULL) {
        perror("fopen");
    }
    else {
        printf("File opened successfully\n");
        fclose(file);
    }

    setuid(getuid());

    printf("\nAfter setuid():\n");
    printf("Real UID: %ld\n", (long)getuid());
    printf("Effective UID: %ld\n", (long)geteuid());

    file = fopen("data.txt", "r");

    if (file == NULL) {
        perror("fopen");
    }
    else {
        printf("File opened successfully!\n");
        fclose(file);
    }

    return 0;
}