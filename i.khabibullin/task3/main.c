#include <stdio.h>
#include <unistd.h>
#include <errno.h>

void print_UIDS()
{
    uid_t real_uid = getuid();
    uid_t eff_uid = geteuid();
    printf("Real UID = %d, Effective UID = %d\n", real_uid, eff_uid);
}

int main(void)
{
    print_UIDS();
    FILE *test_file = fopen("test.txt", "r+");
    if (test_file == NULL)
    {
        perror("First open failed");
    }
    else
    {
        printf("First open succeeded\n");
        fclose(test_file);
    }

    if (setuid(getuid()) == -1)
    {
        perror("Failed to set UID");
        return 2;
    }

    print_UIDS();

    FILE *test_file2 = fopen("test.txt", "r+");
    if (test_file2 == NULL)
    {
        perror("Second open failed");
    }
    else
    {
        printf("Second open succeeded\n");
        fclose(test_file2);
    }

    return 0;
}
