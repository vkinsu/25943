#include <stdio.h>
#include <time.h>
#include <stdlib.h>

int main()
{
    time_t current_time;
    struct tm *california_time;
    char timezone[] = "TZ=PST8";

    putenv(timezone);
    tzset();
    time(&current_time);
    california_time = localtime(&current_time);

    printf("California time: %02d.%02d.%04d %02d:%02d:%02d\n",
           california_time->tm_mday,
           california_time->tm_mon + 1,
           california_time->tm_year + 1900,
           california_time->tm_hour,
           california_time->tm_min,
           california_time->tm_sec);

    return 0;
}
