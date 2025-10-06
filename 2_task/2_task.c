#include <stdio.h>
#include <time.h>
#include <stdlib.h>

int main(){
    // time_t - тип данных хранящий кол-во секунд с 1 янврая 1970
    time_t current_time;
    // указатель на структуру в которой содержится вся информация о времени и флаг летнего времени
    struct tm *pst_time;
    // буфер для строки времени
    char buffer[100];
    
    // Получаем текущее время
    time(&current_time);
    
    // Устанавливаем временную зону для PST (UTC-8)  TZ - timezone, 1 - set value
    setenv("TZ", "PST8PDT", 1);
    // tzset - применяет изменения временной зоны
    tzset();
    
    // Преобразуем в локальное время PST - форматирование для вывода
    pst_time = localtime(&current_time);
    
    // Выводим время в PST
    strftime(buffer, sizeof(buffer), "California (PST) time: %Y-%m-%d %H:%M:%S %Z", pst_time);
    printf("%s\n", buffer);
    
    // Для сравнения выводим UTC время
    struct tm *utc_time = gmtime(&current_time);
    strftime(buffer, sizeof(buffer), "UTC time: %Y-%m-%d %H:%M:%S", utc_time);
    printf("%s\n", buffer);
    
    return 0;
}