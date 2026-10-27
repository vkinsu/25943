#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int main()
{
    FILE *f;

    printf("Real UID: %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());

    f = fopen("data.txt", "r");
    if (f == NULL) {
        perror("fopen");
    } else {
        printf("File opened OK\n");
        fclose(f);
    }

    if (setuid(getuid()) != 0) {
        perror("setuid");
        return 1;
    }

    printf("--- After setuid ---\n");
    printf("Real UID: %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());

    f = fopen("data.txt", "r");
    if (f == NULL) {
        perror("fopen");
    } else {
        printf("File opened OK\n");
        fclose(f);
    }

    return 0;
}
