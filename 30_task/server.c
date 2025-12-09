#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/my_socket"
#define BUFFER_SIZE 1024

int main(){
    int server_fd, client_fd;
    struct sockaddr_un addr;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    // Удаляем старый сокет, если он существует
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

    // Привязываем сокет к адресу
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1)
    {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Начинаем прослушивание
    if (listen(server_fd, 5) == -1)
    {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on %s\n", SOCKET_PATH);

    // Принимаем соединение
    client_fd = accept(server_fd, NULL, NULL);
    if (client_fd == -1)
    {
        perror("accept");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Читаем данные от клиента
    while ((bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1)) > 0)
    {
        buffer[bytes_read] = '\0';
        
        // Преобразуем в верхний регистр и выводим
        for (int i = 0; i < bytes_read; i++)
        {
            buffer[i] = toupper(buffer[i]);
        }
        
        printf("%s", buffer);
        fflush(stdout);
    }

    // Закрываем соединения
    close(client_fd);
    close(server_fd);
    
    // Удаляем файл сокета
    unlink(SOCKET_PATH);
    
    return 0;
}