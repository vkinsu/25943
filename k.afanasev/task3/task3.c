#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

static void check_access(const char *filename)
{
    FILE *stream = fopen(filename, "r");
    if (stream == NULL)
    {
        perror("fopen");
        return;
    }
    printf("fopen(\"%s\") - succesful\n", filename);
    if (fclose(stream) != 0)
    {
        perror("fclose");
    }
}

static void show_identity(const char *stage)
{
    printf("--- %s ---\n", stage);
    printf("real UID      = %d\n", getuid());
    printf("effective UID = %d\n", geteuid());
}

int main(int argument_count, char *arguments[])
{
    const char *filename = (argument_count > 1) ? arguments[1] : "data.txt";

    show_identity("to setuid");
    check_access(filename);

    if (setuid(getuid()) != 0){
        perror("setuid");
        return 1;
    }

    show_identity("after seuid");
    check_access(filename);

    return 0;
}
