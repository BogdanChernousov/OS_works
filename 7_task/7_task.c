#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/select.h>
#include <sys/mman.h>
#include <sys/stat.h>

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

    // Получаем размер файла для mmap
    struct stat sb;
    if (fstat(fd, &sb) == -1)
    {
        perror("Ошибка получения размера файла");
        close(fd);
        return 1;
    }

    size_t file_size = sb.st_size;

    // Отображаем файл в память (ЗАМЕНА read/lseek)
    char *file_data = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_data == MAP_FAILED)
    {
        perror("Ошибка отображения файла");
        close(fd);
        return 1;
    }

    // Строим таблицу строк используя mmap (ЗАМЕНА read)
    long offsets[MAX_LINES];
    int lengths[MAX_LINES];
    int line_count = 0;
    
    offsets[0] = 0;
    int line_length = 0;

    for (size_t i = 0; i < file_size; i++)
    {
        line_length++;
        
        if (file_data[i] == '\n')
        {
            lengths[line_count] = line_length;
            line_count++;
            offsets[line_count] = i + 1;
            line_length = 0;
            
            if (line_count >= MAX_LINES - 1) break;
        }
    }

    if (line_length > 0)
    {
        lengths[line_count] = line_length;
        line_count++;
    }

    printf("Файл '%s' содержит %d строк\n", argv[1], line_count);

    // ВЫВОД ТАБЛИЦЫ ОТСТУПОВ С СОДЕРЖАНИЕМ (точно как в прошлых задачах)
    printf("\n=== ТАБЛИЦА СТРОК ===\n");
    printf("№ строки | Смещение | Длина | Содержимое\n");
    printf("---------|----------|-------|------------\n");
    
    for (int i = 0; i < line_count; i++)
    {
        // Получаем содержимое строки напрямую из памяти
        char *line_start = file_data + offsets[i];
        int line_len = lengths[i];
        
        // Создаем копию для отображения (убираем \n)
        char display_line[line_len + 1];
        strncpy(display_line, line_start, line_len);
        display_line[line_len] = '\0';
        
        // Убираем символ новой строки из отображения
        if (display_line[line_len - 1] == '\n') {
            display_line[line_len - 1] = '\0';
        }
        
        printf("%8d | %8ld | %5d | '%s'\n", 
               i + 1, offsets[i], lengths[i], display_line);
    }
    
    printf("\n");

    printf("У вас %d секунд чтобы ввести номер строки: ", TIMEOUT);
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
    while(1){

        if (result > 0)
        {
            if (flag != -1)
            {
                printf("\nВведите номер строки (0 для выхода): ");
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
                    printf("Строка %d: %.*s", line_num, lengths[line_num - 1], file_data + offsets[line_num - 1]);
                }
                else
                {
                    printf("Ошибка: неверный номер строки\n");
                }
            }
            else
            {
            printf("Ошибка: некорректный ввод. Пожалуйста, введите число.\n");
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            }
        }
        else if (result == 0)
        {
            // Таймаут - время вышло
            printf("\nВремя вышло! Вывод всего файла:\n");
            printf("================================\n");
            
            // Выводим весь файл из памяти (ЗАМЕНА read/write)
            fwrite(file_data, 1, file_size, stdout);
            printf("\n================================\n");
            break;
        }
        else
        {
            perror("Ошибка select");
            break;
        }
    }
    // Освобождаем ресурсы mmap
    munmap(file_data, file_size);
    close(fd);
    
    return 0;
}