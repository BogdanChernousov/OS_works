#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
        return 1;
    }

    pid_t pid = fork();
    
    if (pid == -1)
    {
        perror("fork");
        return 1;
    }
    
    if (pid == 0)
    {
        // Дочерний процесс
        execvp(argv[1], &argv[1]);
        perror("execvp");
        exit(127);
    }
    else
    {
        // Родительский процесс
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status))
        {
            printf("Exit code: %d\n", WEXITSTATUS(status));
            return WEXITSTATUS(status);
        }
        else if (WIFSIGNALED(status))
        {
            printf("Terminated by signal: %d\n", WTERMSIG(status));
            return 128 + WTERMSIG(status);
        }
        else
        {
            printf("Process terminated abnormally\n");
            return 1;
        }
    }
}