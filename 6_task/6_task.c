#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/select.h>

#define MAX_LINES 1000
#define TIMEOUT 5

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

    // Строим таблицу строк
    long offsets[MAX_LINES];
    int lengths[MAX_LINES];
    int line_count = 0;
    
    offsets[0] = 0;
    char buffer[1024];
    ssize_t bytes_read;
    long current_pos = 0;
    int line_length = 0;

    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
    {
        for (int i = 0; i < bytes_read; i++)
        {
            line_length++;
            
            if (buffer[i] == '\n')
            {
                lengths[line_count] = line_length;
                line_count++;
                offsets[line_count] = current_pos + i + 1;
                line_length = 0;
                
                if (line_count >= MAX_LINES - 1) break;
            }
        }
        current_pos += bytes_read;
    }

    if (line_length > 0)
    {
        lengths[line_count] = line_length;
        line_count++;
    }

    printf("Файл '%s' содержит %d строк\n", argv[1], line_count);

    // ВЫВОД ТАБЛИЦЫ ОТСТУПОВ С СОДЕРЖАНИЕМ
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

    printf("У вас %d секунд чтобы ввести номер строки\n", TIMEOUT);
    fflush(stdout);

    // Используем select для таймаута ввода
    fd_set readfds;
    struct timeval timeout;

    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    timeout.tv_sec = TIMEOUT;
    timeout.tv_usec = 0;

    int result = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout);
    int flag = -1;

    while(1)
    {
        if (result > 0)
        {
            if (flag != -1)
            {
                printf("Введите номер строки (0 для выхода): ");
            }
            flag--;
            // Пользователь успел ввести
            int line_num;
            if (scanf("%d", &line_num) == 1)
            {
                if (line_num == 0)
                {
                    printf("Завершение работы\n");
                    break;
                }
                else if (line_num >= 1 && line_num <= line_count)
                {
                    lseek(fd, offsets[line_num - 1], SEEK_SET);
                    char line[lengths[line_num - 1] + 1];
                    read(fd, line, lengths[line_num - 1]);
                    line[lengths[line_num - 1]] = '\0';
                    printf("Строка %d: %s\n", line_num, line);
                }
                else
                {
                    printf("Ошибка: неверный номер строки\n");
                }
            }
            else
            {
                printf("\nОшибка: некорректный ввод. Пожалуйста, введите число.\n");
                int c;
                while ((c = getchar()) != '\n' && c != EOF);
            }
        }
        else if (result == 0)
        {
            // Таймаут - время вышло
            printf("\nВремя вышло! Вывод всего файла:\n");
            printf("================================\n");
                
            lseek(fd, 0, SEEK_SET);
            while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
            {
                write(STDOUT_FILENO, buffer, bytes_read);
            }
            printf("\n================================\n");
            break;
        }
        else
        {
            perror("Ошибка select");
            break;
        }
    }

    close(fd);
    return 0;
}