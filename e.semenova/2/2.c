#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(void)
{
  time_t now;
  struct tm *sp;

  putenv("TZ=PST8");
  tzset();

  (void)time(&now);

  sp = localtime(&now);

  printf("%d/%d/%d %d:%02d %s\n", sp->tm_mon + 1, sp->tm_mday, sp->tm_year + 1900, sp->tm_hour, sp->tm_min, tzname[sp->tm_isdst]);

  return 0;
}
