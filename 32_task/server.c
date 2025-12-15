#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>

#define SOCKET_PATH "/tmp/my_socket"
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

// Структура для хранения информации о клиенте
struct client_info {
    int fd;
    char buffer[BUFFER_SIZE];
    int data_ready;
    size_t data_len;
};

struct client_info clients[MAX_CLIENTS];
int server_fd;
volatile sig_atomic_t new_connection = 0;

// Обработчик SIGIO для асинхронного ввода-вывода
void sigio_handler(int sig) {
    // Устанавливаем флаг, что есть новые данные
    new_connection = 1;
}

// Обработчик SIGINT для корректного завершения
void sigint_handler(int sig) {
    printf("\nShutting down server...\n");
    
    // Закрываем все клиентские соединения
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd != -1) {
            close(clients[i].fd);
        }
    }
    
    // Закрываем серверный сокет
    close(server_fd);
    
    // Удаляем файл сокета
    unlink(SOCKET_PATH);
    
    exit(0);
}

void setup_async_io(int fd) {
    // На Solaris: O_NONBLOCK + FASYNC
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK | FASYNC);
    
    // Устанавливаем владельца для получения сигналов
    fcntl(fd, F_SETOWN, getpid());
}

// Функция для поиска свободного слота
int find_free_slot() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd == -1) {
            return i;
        }
    }
    return -1;
}

// Функция для обработки данных от клиента
void process_client_data(int index) {
    if (clients[index].fd == -1) return;
    
    // Читаем данные из сокета
    ssize_t bytes_read = read(clients[index].fd, clients[index].buffer, BUFFER_SIZE - 1);
    
    if (bytes_read > 0) {
        // Преобразуем в верхний регистр
        for (int j = 0; j < bytes_read; j++) {
            clients[index].buffer[j] = toupper((unsigned char)clients[index].buffer[j]);
        }
        
        clients[index].buffer[bytes_read] = '\0';
        printf("[Client %d]: %s", clients[index].fd, clients[index].buffer);
        fflush(stdout);
    } else if (bytes_read == 0) {
        // Клиент отключился
        printf("Client (fd=%d) disconnected\n", clients[index].fd);
        close(clients[index].fd);
        clients[index].fd = -1;
    } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
        // Ошибка чтения
        perror("read");
        close(clients[index].fd);
        clients[index].fd = -1;
    }
}

int main() {
    struct sockaddr_un addr;
    struct sigaction sa_io, sa_int;
    
    // Инициализируем массив клиентов
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;
        clients[i].data_ready = 0;
    }

    // Удаляем старый сокет
    unlink(SOCKET_PATH);

    // Создаем сокет
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Настраиваем асинхронный ввод-вывод для серверного сокета
    setup_async_io(server_fd);

    // Настраиваем адрес
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    // Привязываем сокет
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Начинаем прослушивание
    if (listen(server_fd, MAX_CLIENTS) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Async IO server is listening on %s\n", SOCKET_PATH);
    printf("Press Ctrl+C to exit\n");
    
    // Настраиваем обработчик сигнала SIGIO
    memset(&sa_io, 0, sizeof(struct sigaction));
    sa_io.sa_handler = sigio_handler;
    sigemptyset(&sa_io.sa_mask);
    sa_io.sa_flags = SA_RESTART;
    
    if (sigaction(SIGIO, &sa_io, NULL) == -1) {
        perror("sigaction SIGIO");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    // Настраиваем обработчик SIGINT
    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;
    
    if (sigaction(SIGINT, &sa_int, NULL) == -1) {
        perror("sigaction SIGINT");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server ready. Waiting for connections...\n");

    // Основной цикл сервера
    while (1) {
        // Если был сигнал SIGIO, обрабатываем активность
        if (new_connection) {
            new_connection = 0;
            
            // Пробуем принять новые соединения
            int client_fd;
            while ((client_fd = accept(server_fd, NULL, NULL)) != -1) {
                int slot = find_free_slot();
                
                if (slot == -1) {
                    printf("No free slots, rejecting client (fd=%d)\n", client_fd);
                    close(client_fd);
                    continue;
                }
                
                // Настраиваем асинхронный ввод-вывод для клиента
                setup_async_io(client_fd);
                
                // Сохраняем информацию о клиенте
                clients[slot].fd = client_fd;
                clients[slot].data_ready = 0;
                
                printf("New client connected (fd=%d)\n", client_fd);
            }
            
            // Проверяем данные от существующих клиентов
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].fd != -1) {
                    process_client_data(i);
                }
            }
        }
        
        // Периодически проверяем клиентов
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].fd != -1) {

                fd_set read_fds;
                FD_ZERO(&read_fds);
                FD_SET(clients[i].fd, &read_fds);
                
                struct timeval tv = {0, 0};
                if (select(clients[i].fd + 1, &read_fds, NULL, NULL, &tv) > 0) {
                    if (FD_ISSET(clients[i].fd, &read_fds)) {
                        process_client_data(i);
                    }
                }
            }
        }
        usleep(10000); // 10ms
    }
    return 0;
}