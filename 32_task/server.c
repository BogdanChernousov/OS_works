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
#include <sys/file.h>  // Для FASYNC на Solaris

#define SOCKET_PATH "/tmp/my_socket"
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

struct client_info {
    int fd;
    char buffer[BUFFER_SIZE];
    int buf_pos;
    int active;
};

struct client_info clients[MAX_CLIENTS];
int server_fd;
volatile sig_atomic_t data_ready = 0;

// Обработчик SIGIO - вызывается при активности на любом из сокетов
void sigio_handler(int sig) {
    data_ready = 1;  // Просто устанавливаем флаг
}

// Обработчик SIGINT
void sigint_handler(int sig) {
    printf("\nЗавершение работы сервера...\n");
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            close(clients[i].fd);
        }
    }
    
    close(server_fd);
    unlink(SOCKET_PATH);
    exit(0);
}

// Настройка асинхронного ввода-вывода для сокета
void setup_async_io(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK | FASYNC);  // FASYNC для Solaris
    fcntl(fd, F_SETOWN, getpid());  // Отправлять сигналы этому процессу
}

// Основная функция обработки данных
void process_all_clients() {
    char ch;
    int i;
    
    // 1. Проверяем новые подключения (неблокирующий accept)
    int client_fd;
    while ((client_fd = accept(server_fd, NULL, NULL)) != -1) {
        // Находим свободный слот
        int slot = -1;
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (!clients[i].active) {
                slot = i;
                break;
            }
        }
        
        if (slot == -1) {
            printf("Достигнут лимит клиентов (%d), отказ\n", MAX_CLIENTS);
            close(client_fd);
            continue;
        }
        
        // Настраиваем асинхронный ввод-вывод для нового клиента
        setup_async_io(client_fd);
        
        clients[slot].fd = client_fd;
        clients[slot].active = 1;
        clients[slot].buf_pos = 0;
        
        printf("Новый клиент подключен (fd=%d)\n", client_fd);
    }
    
    // 2. Обрабатываем данные от всех активных клиентов
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].active) continue;
        
        ssize_t bytes_read;
        char temp_buf[BUFFER_SIZE];
        
        // Читаем всё доступное (неблокирующее чтение)
        while ((bytes_read = read(clients[i].fd, temp_buf, BUFFER_SIZE - 1)) > 0) {
            // Преобразуем в верхний регистр и выводим ПОСИМВОЛЬНО
            for (int j = 0; j < bytes_read; j++) {
                // Немедленный вывод символа - это создает смешивание!
                putchar(toupper((unsigned char)temp_buf[j]));
                fflush(stdout);  // Важно для немедленного вывода
            }
        }
        
        // Проверяем отключение клиента
        if (bytes_read == 0) {
            printf("\nКлиент отключен (fd=%d)\n", clients[i].fd);
            close(clients[i].fd);
            clients[i].active = 0;
        }
    }
}

int main() {
    struct sockaddr_un addr;
    struct sigaction sa_io, sa_int;
    
    // Инициализация
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].active = 0;
        clients[i].fd = -1;
        clients[i].buf_pos = 0;
    }
    
    // Удаляем старый сокет если существует
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
    
    printf("АСИНХРОННЫЙ СЕРВЕР\n");
    printf("Сокет: %s\n", SOCKET_PATH);
    printf("Максимум клиентов: %d\n", MAX_CLIENTS);
    printf("Нажмите Ctrl+C для выхода\n\n");
    
    // Настраиваем обработчик SIGIO
    memset(&sa_io, 0, sizeof(struct sigaction));
    sa_io.sa_handler = sigio_handler;
    sigemptyset(&sa_io.sa_mask);
    sa_io.sa_flags = SA_RESTART;  // Перезапускать системные вызовы
    
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
    
    printf("Сервер запущен и ожидает подключений...\n");
    
    // Основной цикл - реагирует на флаг от сигнала
    while (1) {
        if (data_ready) {
            data_ready = 0;
            process_all_clients();
        }
        
        // Небольшая пауза чтобы не грузить CPU
        // Но достаточно большая, чтобы сигналы могли прерывать
        usleep(1000);  // 1ms
    }
    
    return 0;
}