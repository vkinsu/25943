#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    int fd;
    struct flock lock;

    if (argc < 2) {
        printf("Usage: %s file\n", argv[0]);
        return 1;
    }

    fd = open(argv[1], O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    memset(&lock, 0, sizeof(lock));
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;

    if (fcntl(fd, F_SETLK, &lock) < 0) {
        perror("fcntl lock");
        close(fd);
        return 1;
    }
    printf("File locked. Starting editor...\n");

    system("vi test_lock.txt");

    lock.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &lock);
    printf("File unlocked. Bye.\n");

    close(fd);
    return 0;
}

