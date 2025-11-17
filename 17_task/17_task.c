#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#define MAX_LINE 40
#define ERASE 0x7F
#define KILL 0x15
#define CTRL_W 0x17
#define CTRL_D 0x04

struct termios orig_attrs;

void restore_terminal(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_attrs);
}

void setup_terminal(void) {
    struct termios attrs;
    tcgetattr(STDIN_FILENO, &orig_attrs);
    atexit(restore_terminal);
    
    attrs = orig_attrs;
    attrs.c_lflag &= ~(ICANON | ECHO);
    attrs.c_cc[VMIN] = 1;
    attrs.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &attrs);
}

int is_printable(char c) {
    return c >= 32 && c <= 126;
}

int find_last_space(char *line, int pos) {
    for (int i = pos - 1; i >= 0; i--) {
        if (line[i] == ' ') return i;
    }
    return -1;
}

int find_word_start(char *line, int pos) {
    int i = pos - 1;
    while (i >= 0 && line[i] == ' ') i--;
    while (i >= 0 && line[i] != ' ') i--;
    return i + 1;
}

int main(void) {
    char line[1024] = {0};
    int pos = 0;
    int current_line_length = 0;
    char c;
    
    setup_terminal();
    printf("Line editor (Ctrl+D to exit): ");
    fflush(stdout);
    current_line_length = 24; // Prompt length

    while (1) {
        read(STDIN_FILENO, &c, 1);
        
        // Exit on Ctrl+D at line start
        if (c == CTRL_D && pos == 0) {
            printf("\n");
            break;
        }
        
        // Ignore non-printable chars
        if (!is_printable(c) && c != ERASE && c != KILL && c != CTRL_W) {
            write(STDOUT_FILENO, "\a", 1);
            continue;
        }
        
        switch (c) {
            case ERASE: // Backspace
                if (pos > 0) {
                    pos--;
                    current_line_length--;
                    printf("\b \b");
                    fflush(stdout);
                }
                break;
                
            case KILL: // Clear line
                while (pos > 0) {
                    pos--;
                    printf("\b \b");
                }
                current_line_length = 0;
                fflush(stdout);
                break;
                
            case CTRL_W: // Delete word
                if (pos > 0) {
                    int word_start = find_word_start(line, pos);
                    for (int i = pos; i > word_start; i--) {
                        printf("\b \b");
                        current_line_length--;
                    }
                    pos = word_start;
                    fflush(stdout);
                }
                break;
                
            default: // Normal character
                line[pos++] = c;
                
                // Line wrapping logic
                if (current_line_length >= MAX_LINE) {
                    int last_space = find_last_space(line, pos);
                    
                    if (last_space != -1) {
                        int word_start = last_space + 1;
                        int word_length = pos - word_start;
                        
                        if (word_length < MAX_LINE) {
                            // Wrap whole word
                            printf("\n");
                            for (int i = word_start; i < pos; i++) {
                                write(STDOUT_FILENO, &line[i], 1);
                            }
                            current_line_length = word_length;
                        } else {
                            // Break long word
                            printf("\n");
                            write(STDOUT_FILENO, &c, 1);
                            current_line_length = 1;
                        }
                    } else {
                        // No spaces - break at char
                        printf("\n");
                        write(STDOUT_FILENO, &c, 1);
                        current_line_length = 1;
                    }
                } else {
                    // Normal output
                    write(STDOUT_FILENO, &c, 1);
                    current_line_length++;
                }
        }
        line[pos] = '\0';
    }
    
    return 0;
}