#include <stdio.h>
#include <time.h>
#include <stdlib.h>

int main()
{
    time_t now;
    struct tm my_time;

    setenv("TZ", "PST8PDT", 1);
    tzset();

    time(&now);

    printf("%s", ctime(&now));

    my_time = *localtime(&now);

    printf("%d/%d/%d %d:%02d %s\n",
        my_time.tm_mon + 1,
        my_time.tm_mday,
        my_time.tm_year + 1900,
        my_time.tm_hour,
        my_time.tm_min,
        my_time.tm_isdst ? "PDT" : "PST");

    return 0;
}
