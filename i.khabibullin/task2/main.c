#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

const char *DAYS[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const char *MONTHS[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

int main()
{
    time_t now;
    struct tm *sp;

    if (putenv("TZ=PST8") != 0)
    {
        perror("Error changing TZ");
        exit(1);
    }

    tzset();

    if (time(&now) == (time_t)-1)
    {
        perror("Error getting time");
        exit(1);
    }

    sp = localtime(&now);
    if (sp == NULL)
    {
        perror("Error converting time");
        exit(1);
    }

    printf("%s %s %2d %02d:%02d:%02d PST %d\n",
           DAYS[sp->tm_wday],
           MONTHS[sp->tm_mon],
           sp->tm_mday,
           sp->tm_hour,
           sp->tm_min,
           sp->tm_sec,
           sp->tm_year + 1900);

    exit(0);
}
