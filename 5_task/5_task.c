#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define MAX_LINES 1000

int main(int argc, char *argv[]){
    if (argc != 2)
    {
        fprintf(stderr, "Использование: %s <файл>\n", argv[0]);
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1)
    {
        perror("Ошибка открытия файла");
        return 1;
    }

    // Таблицы для смещений и длин строк
    long offsets[MAX_LINES];
    int lengths[MAX_LINES];
    int line_count = 0;
    
    offsets[0] = 0;  // Первая строка начинается с позиции 0
    char buffer[1024];
    ssize_t bytes_read;
    long current_pos = 0;
    int line_length = 0;

    printf("Построение таблицы строк...\n");
    
    // Читаем файл и строим таблицу строк
    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
    {
        for (int i = 0; i < bytes_read; i++)
        {
            line_length++;
            
            if (buffer[i] == '\n')
            {
                // Сохраняем информацию о строке
                lengths[line_count] = line_length;
                line_count++;
                offsets[line_count] = current_pos + i + 1;  // Начало следующей строки
                line_length = 0;
                
                if (line_count >= MAX_LINES - 1)
                {
                    printf("Достигнут предел количества строк\n");
                    break;
                }
            }
        }
        current_pos += bytes_read;
    }

    // Обрабатываем последнюю строку если она не заканчивается \n
    if (line_length > 0)
    {
        lengths[line_count] = line_length;
        line_count++;
    }

    printf("Файл '%s' содержит %d строк\n", argv[1], line_count);

    // ВЫВОД ТАБЛИЦЫ СМЕЩЕНИЙ И ДЛИН СТРОК
    printf("\n=== ТАБЛИЦА СТРОК ===\n");
    printf("№ строки | Смещение | Длина | Содержимое\n");
    printf("---------|----------|-------|------------\n");
    
    // Сохраняем текущую позицию в файле
    long saved_pos = lseek(fd, 0L, SEEK_CUR);
    
    for (int i = 0; i < line_count; i++)
    {
        // Перемещаемся к строке и читаем её для отображения
        lseek(fd, offsets[i], SEEK_SET);
        char line_content[lengths[i] + 1];
        read(fd, line_content, lengths[i]);
        line_content[lengths[i]] = '\0';
        
        // Убираем символ новой строки из отображения
        if (line_content[lengths[i] - 1] == '\n') {
            line_content[lengths[i] - 1] = '\0';
        }
        
        printf("%8d | %8ld | %5d | '%s'\n", 
               i + 1, offsets[i], lengths[i], line_content);
    }
    
    // Восстанавливаем позицию в файле
    lseek(fd, saved_pos, SEEK_SET);
    
    printf("\n");

    // Основной цикл программы
    int line_num;
    while (1)
    {
        printf("Введите номер строки (0 для выхода): ");
        
        if (scanf("%d", &line_num) != 1)
        {
            printf("Ошибка ввода\n");
            while (getchar() != '\n'); // Очистка буфера
            continue;
        }

        if (line_num == 0)
        {
            printf("Завершение работы\n");
            break;
        }

        if (line_num < 1 || line_num > line_count)
        {
            printf("Ошибка: строка %d не существует\n", line_num);
            continue;
        }

        // Перемещаемся к нужной строке и читаем её
        lseek(fd, offsets[line_num - 1], SEEK_SET);
        char line[lengths[line_num - 1] + 1];
        read(fd, line, lengths[line_num - 1]);
        line[lengths[line_num - 1]] = '\0';
        
        printf("Строка %d: %s\n", line_num, line);
    }

    close(fd);
    return 0;
}