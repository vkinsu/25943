#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

void check_access(const char *filename) {
    printf("Real UID: %d, Effective UID: %d\n", getuid(), geteuid());
    
    FILE *file = fopen(filename, "r");
    if (file != NULL) {
        printf("Successfully opened '%s'\n", filename);
        fclose(file);
    } else {
        perror("fopen error");
    }
}

int main(int argc, char *argv[]) {
    const char *filename = (argc > 1) ? argv[1] : "file.txt";

    printf("First attempt:\n");
    check_access(filename);

    printf("\nDropping effective privileges: setuid(getuid()):\n");
    if (setuid(getuid()) != 0) {
        perror("setuid error");
    }

    printf("\nSecond attempt:\n");
    check_access(filename);

    return 0;
}
