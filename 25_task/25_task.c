#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>

#define BUFFER_SIZE 1024

void parent_process(int pipefd[]) {
    close(pipefd[0]); // Close read end
    
    char *text = "Hello World! Test test TEST ^[D";
    
    // Write text to pipe
    write(pipefd[1], text, strlen(text) + 1);
    close(pipefd[1]); // Close write end
    
    // Wait for child
    wait(NULL);
}

void child_process(int pipefd[]) {
    close(pipefd[1]); // Close write end
    
    char buffer[BUFFER_SIZE];
    int bytes_read;
    
    // Read from pipe
    bytes_read = read(pipefd[0], buffer, BUFFER_SIZE - 1);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        
        // Convert to uppercase
        for (int i = 0; i < bytes_read; i++) {
            buffer[i] = toupper(buffer[i]);
        }
        
        // Print result
        printf("Converted text: %s\n", buffer);
    }
    
    close(pipefd[0]); // Close read end
}

int main() {
    int pipefd[2];
    pid_t pid;
    
    // Create pipe
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    
    // Create child process
    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    
    if (pid > 0) {
        // Parent process
        parent_process(pipefd);
    } else {
        // Child process
        child_process(pipefd);
    }
    
    return 0;
}