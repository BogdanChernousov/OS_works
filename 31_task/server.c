#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>

#define SOCKET_PATH "/tmp/my_socket"
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

int main(){
    int server_fd, client_fd, max_fd;
    struct sockaddr_un addr;
    fd_set read_fds, master_fds;
    int client_fds[MAX_CLIENTS];
    int i, n;
    char buffer[BUFFER_SIZE];

    // Инициализируем массив клиентов
    for (i = 0; i < MAX_CLIENTS; i++)
    {
        client_fds[i] = -1;
    }

    // Удаляем старый сокет
    unlink(SOCKET_PATH);

    // Создаем сокет
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Настраиваем адрес
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    // Привязываем сокет
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1)
    {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Начинаем прослушивание
    if (listen(server_fd, MAX_CLIENTS) == -1)
    {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Multi-client server is listening on %s\n", SOCKET_PATH);

    // Инициализируем наборы файловых дескрипторов
    FD_ZERO(&master_fds);
    FD_SET(server_fd, &master_fds);
    max_fd = server_fd;

    while (1)
    {
        read_fds = master_fds;
        
        // Используем select для мультиплексирования
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) == -1)
        {
            perror("select");
            exit(EXIT_FAILURE);
        }

        // Проверяем все файловые дескрипторы
        for (i = 0; i <= max_fd; i++)
        {
            if (FD_ISSET(i, &read_fds))
            {
                if (i == server_fd)
                {
                    // Новое соединение
                    client_fd = accept(server_fd, NULL, NULL);
                    if (client_fd == -1)
                    {
                        perror("accept");
                        continue;
                    }

                    // Добавляем нового клиента
                    FD_SET(client_fd, &master_fds);
                    if (client_fd > max_fd)
                    {
                        max_fd = client_fd;
                    }

                    printf("New client connected (fd=%d)\n", client_fd);
                }
                else
                {
                    // Данные от существующего клиента
                    ssize_t bytes_read = read(i, buffer, BUFFER_SIZE - 1);
                    
                    if (bytes_read <= 0)
                    {
                        // Соединение закрыто
                        if (bytes_read == 0)
                        {
                            printf("Client (fd=%d) disconnected\n", i);
                        }
                        else
                        {
                            perror("read");
                        }
                        
                        close(i);
                        FD_CLR(i, &master_fds);
                        
                        // Удаляем клиента из массива
                        for (int j = 0; j < MAX_CLIENTS; j++)
                        {
                            if (client_fds[j] == i)
                            {
                                client_fds[j] = -1;
                                break;
                            }
                        }
                    }
                    else
                    {
                        // Обрабатываем данные
                        buffer[bytes_read] = '\0';
                        
                        // Преобразуем в верхний регистр
                        for (int j = 0; j < bytes_read; j++)
                        {
                            buffer[j] = toupper(buffer[j]);
                        }
                        
                        // Выводим результат
                        printf("[Client %d]: %s", i, buffer);
                        fflush(stdout);
                    }
                }
            }
        }
    }

    // Закрываем серверный сокет
    close(server_fd);
    unlink(SOCKET_PATH);
    
    return 0;
}