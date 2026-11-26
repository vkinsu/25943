#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <string.h>

void print_file(const char *path)
{
    struct stat st;

    if (lstat(path, &st) == -1) 
    {
        perror(path);
        return;
    }

    char permissions[11];

    if (S_ISDIR(st.st_mode))
        permissions[0] = 'd';
    else if (S_ISREG(st.st_mode))
        permissions[0] = '-';
    else
        permissions[0] = '?';

    permissions[1] = (st.st_mode & S_IRUSR) ? 'r' : '-';
    permissions[2] = (st.st_mode & S_IWUSR) ? 'w' : '-';
    permissions[3] = (st.st_mode & S_IXUSR) ? 'x' : '-';

    permissions[4] = (st.st_mode & S_IRGRP) ? 'r' : '-';
    permissions[5] = (st.st_mode & S_IWGRP) ? 'w' : '-';
    permissions[6] = (st.st_mode & S_IXGRP) ? 'x' : '-';

    permissions[7] = (st.st_mode & S_IROTH) ? 'r' : '-';
    permissions[8] = (st.st_mode & S_IWOTH) ? 'w' : '-';
    permissions[9] = (st.st_mode & S_IXOTH) ? 'x' : '-';

    permissions[10] = '\0';

    struct passwd *owner = getpwuid(st.st_uid);
    struct group *group = getgrgid(st.st_gid);

    char *owner_name = owner ? owner->pw_name : "unknown";
    char *group_name = group ? group->gr_name : "unknown";

    char date[20];
    struct tm *time_info = localtime(&st.st_mtime);

    strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", time_info);

    const char *name = strrchr(path, '/');

    if (name != NULL)
        name++;
    else
        name = path;

    printf("%-10s %-7lu %-10s %-10s ",
           permissions,
           (unsigned long)st.st_nlink,
           owner_name,
           group_name);

    if (S_ISREG(st.st_mode))
        printf("%-10lld ", (long long)st.st_size);
    else
        printf("%-10s ", "");

    printf("%-20s %s\n", date, name);
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("Usage: %s file...\n", argv[0]);
        return 1;
    }

    printf("%-10s %-7s %-10s %-10s %-10s %-20s %s\n",
           "Permissions", "Links", "Owner", "Group",
           "Size", "Modified", "Name");

    for (int i = 1; i < argc; i++) 
    {
        print_file(argv[i]);
    }

    return 0;
}