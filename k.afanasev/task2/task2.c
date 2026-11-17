#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

extern char *tzname[];

int main(void)
{
    time_t current_time;
    struct tm *local_time;

    setenv("TZ", "PST8", 1);
    tzset();

    (void) time(&current_time);

    printf("%s", ctime(&current_time));

    local_time = localtime(&current_time);
    printf("%d/%d/%02d %d:%02d %s \n", local_time->tm_mon + 1, local_time->tm_mday, local_time->tm_year+1900, local_time->tm_hour, local_time->tm_min, tzname[local_time->tm_isdst]);
    exit(0);
    return 0;
}
