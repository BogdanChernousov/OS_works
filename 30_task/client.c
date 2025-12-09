#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/my_socket"
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]){
    int sock_fd;
    struct sockaddr_un addr;
    char buffer[BUFFER_SIZE];
    FILE *file = NULL;
    
    // Открываем файл
    file = fopen(argv[1], "r");

    // Создаем сокет
    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd == -1)
    {
        perror("socket");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // Настраиваем адрес
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    // Подключаемся к серверу
    if (connect(sock_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1)
    {
        perror("connect");
        fclose(file);
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    printf("Connected to server\n");

    // Читаем из файла и отправляем на сервер
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0)
    {
        if (write(sock_fd, buffer, bytes_read) == -1)
        {
            perror("write");
            break;
        }
    }
    write(sock_fd, "\n", 1);

    printf("File sent\n");

    // Закрываем файл и соединение
    fclose(file);
    close(sock_fd);
    
    return 0;
}