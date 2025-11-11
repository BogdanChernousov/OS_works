#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

int main() {
    pid_t pid;
    int status;
    
    // Создаем подпроцесс
    pid = fork();
    
    if (pid == -1)
    {
        perror("fork failed");
        exit(1);
    }
    else if (pid == 0)
    {
        // Код дочернего процесса
        printf("Дочерний процесс: PID = %d, PPID = %d\n", getpid(), getppid());
        printf("---------------textstart---------------\n");
        fflush(stdout);
        // Исполняем команду cat для текстового файла
        execlp("cat", "cat", "text.txt", NULL);
        
        // Если execlp вернул управление - ошибка
        perror("execlp failed");
        exit(1);
    }
    else
    {
        // Код родительского процесса
        printf("Родительский процесс: PID = %d, создал дочерний с PID = %d\n", getpid(), pid);
        
        // Часть 1: Без ожидания
        printf("=== Часть 1: Без ожидания ===\n");
        printf("Родитель: Это сообщение выводится сразу\n");
        sleep(1);
        
        // Часть 2: С ожиданием
        printf("\n---------------textend---------------");
        printf("\n=== Часть 2: С ожиданием ===\n");
        printf("Родитель: Ожидаем завершения дочернего процесса...\n");
        
        // Ожидаем завершения дочернего процесса
        pid_t terminated_pid = waitpid(pid, &status, 0);
        
        if (terminated_pid == -1)
        {
            perror("waitpid failed");
            exit(1);
        }
        
        // Анализируем статус завершения
        if (WIFEXITED(status))
        {
            printf("Родитель: Дочерний процесс %d завершился нормально с кодом: %d\n", terminated_pid, WEXITSTATUS(status));
        }
        else if (WIFSIGNALED(status))
        {
            printf("Родитель: Дочерний процесс %d убит сигналом: %d\n", terminated_pid, WTERMSIG(status));
        }
        
        // Финальное сообщение ПОСЛЕ завершения дочернего процесса
        printf("Родитель: ЭТО ПОСЛЕДНЕЕ СООБЩЕНИЕ ВЫВЕДЕНО ПОСЛЕ завершения дочернего процесса\n");
    }
    
    return 0;
}