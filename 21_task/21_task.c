#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

int beep_count = 0;

void handle_signal(int sig) {
    if (sig == SIGINT) {
        beep_count++;
        write(STDOUT_FILENO, "\a", 1);  // Звуковой сигнал
        
        // ВАЖНО: переустановить обработчик
        signal(SIGINT, handle_signal);
    } else if (sig == SIGQUIT) {
        printf("\nПрограмма завершена. Всего звуковых сигналов: %d\n", beep_count);
        exit(0);
    }
}

int main() {
    printf("Программа запущена (PID: %d)\n", getpid());
    printf("Ctrl+C для звукового сигнала\n");
    printf("Ctrl+\\ для завершения программы\n");
    
    // Установить обработчики
    signal(SIGINT, handle_signal);
    signal(SIGQUIT, handle_signal);
    
    while(1) {
        pause();
    }
    
    return 0;
}