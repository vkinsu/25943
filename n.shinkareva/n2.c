#include <sys/types.h> //для системных типов данных
#include <stdio.h>
#include <time.h> //для работы с временем
#include <stdlib.h> //для putenv()

int main(void)
{
    time_t now; //количество секунд, прошедших с 1 января 1970 года
    struct tm *sp; //указатель на структуру с датой и временем

    (void) time(&now); //получаем текущее время и записываем его в now


    // зимнее время Калифорнии — PST (UTC-8)
    putenv("TZ=PST8"); //переменная окружения TZ для часового пояса Калифорнии
    tzset(); //обновляем информацию о часовом поясе

    sp = localtime(&now); //преобразуем now в локальное время и записываем указатель на struct tm в sp

    printf("PST: %02d/%02d/%04d %02d:%02d:%02d\n",
           sp->tm_mon + 1, //в tm месяцы идут от 0 до 11, поэтому +1
           sp->tm_mday, //число
           sp->tm_year + 1900, //tm_year хранит число лет после 1900 года, поэтому +1900
           sp->tm_hour, //часы
           sp->tm_min, //минуты
           sp->tm_sec); //секунды


    // летнее время Калифорнии — PDT (UTC-7)
    putenv("TZ=PDT7");
    tzset();

    sp = localtime(&now);

    printf("PDT: %02d/%02d/%04d %02d:%02d:%02d\n",
           sp->tm_mon + 1,
           sp->tm_mday,
           sp->tm_year + 1900,
           sp->tm_hour,
           sp->tm_min,
           sp->tm_sec);

    return 0;
}