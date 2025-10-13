#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

// Объявляем внешнюю переменную tzname
extern char *tzname[];

int main()
{
    time_t now;
    struct tm *sp;
    
    // Получаем текущее время
    (void) time( &now );
    
    // Устанавливаем временную зону для Калифорнии (PST)
    setenv("TZ", "PST8PDT", 1);
    tzset();
    
    // Выводим время в формате ctime для PST (как в оригинале)
    printf("%s", ctime( &now ) );
    
    // Получаем локальное время для PST
    sp = localtime(&now);
    
    // Выводим время в формате Калифорнии (PST) - как в оригинале
    printf("%d/%d/%02d %d:%02d %s\n",
        sp->tm_mon + 1, sp->tm_mday,
        sp->tm_year + 1900,  // Исправлен год
        sp->tm_hour,
        sp->tm_min, tzname[sp->tm_isdst]);
    
    exit(0);
}