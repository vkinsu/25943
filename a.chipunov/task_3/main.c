#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

static void try_open(const char *name)
{
    FILE *f;

    printf("uid=%ld euid=%ld\n", (long)getuid(), (long)geteuid());

    f = fopen(name, "r");
    if (f == NULL) {
        perror(name);
        return;
    }
    printf("%s: opened\n", name);
    fclose(f);
}

int main(int argc, char *argv[])
{
    const char *name = (argc > 1) ? argv[1] : "file";

    try_open(name);

    /* make euid equal to real uid */
    if (setuid(getuid()) == -1)
        perror("setuid");

    try_open(name);
    return 0;
}
