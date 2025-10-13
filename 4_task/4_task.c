#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

struct Node{
    char* data;
    struct Node* next;
};

struct Node* create_node(const char* str){
    struct Node* new_node = (struct Node*)malloc(sizeof(struct Node));
    if (new_node == NULL) return NULL;
    
    new_node->data = (char*)malloc(strlen(str) + 1);
    if (new_node->data == NULL) {
        free(new_node);
        return NULL;
    }
    
    strcpy(new_node->data, str);
    new_node->next = NULL;
    return new_node;
}

void append_node(struct Node** head, struct Node* new_node) {
    if (*head == NULL) {
        *head = new_node;
    } else {
        struct Node* current = *head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_node;
    }
}

void free_list(struct Node* head) {
    struct Node* current = head;
    while (current != NULL) {
        struct Node* next = current->next;
        free(current->data);
        free(current);
        current = next;
    }
}

void print_list(struct Node* head) {
    struct Node* current = head;
    while (current != NULL) {
        printf("%s", current->data);
        current = current->next;
    }
}

// Функция для фильтрации управляющих символов
void filter_control_chars(char* str) {
    char* src = str;
    char* dst = str;
    
    while (*src) {
        if (*src >= 32 || *src == '\n' || *src == '\t') {
            *dst++ = *src;
        }
        src++;
    }
    *dst = '\0';
}

int main(){
    struct Node* head = NULL;
    char buffer[1024];
    
    printf("Вводите строки (точка в начале строки для завершения):\n");
    
    while (1) {
        printf("> ");
        fflush(stdout);
        
        if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            break;
        }
        
        // Фильтруем управляющие символы
        filter_control_chars(buffer);
        
        // Проверяем, не введена ли точка в начале строки
        if (buffer[0] == '.') {
            break;
        }
        
        // Создаем новый узел
        struct Node* new_node = create_node(buffer);
        if (new_node == NULL) {
            printf("Ошибка создания узла\n");
            free_list(head);
            return 1;
        }
        
        // Добавляем узел в список
        append_node(&head, new_node);
    }
    
    // Выводим все строки из списка
    printf("\nВведенные строки:\n");
    print_list(head);
    
    // Освобождаем память
    free_list(head);
    
    return 0;
}