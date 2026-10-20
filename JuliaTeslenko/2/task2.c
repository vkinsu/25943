#include <stdio.h>
#include <time.h>

int main(void)
{
    time_t t;
    struct tm *tm;

    time(&t);
    tm = gmtime(&t);

    tm->tm_hour -= 8;

    if (tm->tm_hour < 0) {
        tm->tm_hour += 24;
        tm->tm_mday--;
    }

    printf("%s", asctime(tm));
    printf("%d/%d/%02d %02d:%02d:%02d PST\n",
           tm->tm_mon + 1,
           tm->tm_mday,
           tm->tm_year % 100,
           tm->tm_hour,
           tm->tm_min,
           tm->tm_sec);

    return 0;
}

