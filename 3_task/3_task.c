#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>

// Печатает реальный и эффективный идентификаторы пользователя
// Реальный UID - пользователь, который запустил программу
// Эффективный UID - пользователь, от имени которого выполняется программа
void print_user_ids()
{
    printf("Real user ID: %d\n", getuid());
    printf("Effective user ID: %d\n", geteuid());
}

// Пытается открыть файл и возвращает результат операции
// filename - имя файла для открытия
// Возвращает 0 при успехе, -1 при ошибке
int try_open_file(const char* filename)
{
    printf("Attempting to open file: %s\n", filename);
    
    // Пытаемся открыть файл в режиме чтения
    FILE* file = fopen(filename, "r");

    if (file == NULL)
    {
        // Если fopen вернул NULL, выводим сообщение об ошибке
        perror("fopen failed");
        return -1;
    }
    else
    {
        printf("File opened successfully\n");
        fclose(file);  // Закрываем файл после успешного открытия
        return 0;
    }
}

int main(){

    const char* filename = "secure_file.txt";
    
    // Шаг 1: Печатаем начальные идентификаторы пользователя
    printf("=== STEP 1: Initial state ===\n");
    print_user_ids();
    
    // Шаг 2: Пытаемся открыть файл с текущими привилегиями
    printf("\n=== STEP 2: Trying to open file ===\n");
    int result1 = try_open_file(filename);
    
    // Шаг 3: Устанавливаем эффективный UID равным реальному UID
    // Это сбрасывает повышенные привилегии, если они были
    printf("\n=== STEP 3: Setting effective UID to real UID ===\n");
    uid_t real_uid = getuid();
    
    if (setuid(real_uid) == -1)
    {
        perror("setuid failed");
        exit(1);
    }
    
    printf("After setuid():\n");
    print_user_ids();
    
    // Шаг 4: Пытаемся открыть файл снова после сброса привилегий
    printf("\n=== STEP 4: Trying to open file again ===\n");
    int result2 = try_open_file(filename);
    
    // Итог: сравниваем результаты двух попыток открытия файла
    printf("\n=== SUMMARY ===\n");
    printf("First attempt: %s\n", result1 == 0 ? "SUCCESS" : "FAILED");
    printf("Second attempt: %s\n", result2 == 0 ? "SUCCESS" : "FAILED");
    
    return 0;
}


// sudo -u test_user /home/bogdan-chernousov/Documents/C_OS/C_Projects/3_task/file_access