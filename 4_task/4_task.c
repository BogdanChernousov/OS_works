#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Структура для узла списка
struct Node{
    char* data;
    struct Node* next;
};

// Функция для создания нового узла
struct Node* create_node(const char* str){
    // Выделяем память для узла
    struct Node* new_node = (struct Node*)malloc(sizeof(struct Node));
    if (new_node == NULL)
    {
        printf("Ошибка выделения памяти для узла\n");
        return NULL;
    }
    
    // Выделяем память для строки (+1 для нулевого символа)
    new_node->data = (char*)malloc(strlen(str) + 1);
    if (new_node->data == NULL)
    {
        printf("Ошибка выделения памяти для строки\n");
        free(new_node);
        return NULL;
    }
    
    // Копируем строку
    strcpy(new_node->data, str);
    new_node->next = NULL;
    
    return new_node;
}

// Функция для добавления узла в конец списка
void append_node(struct Node** head, struct Node* new_node)
{
    if (*head == NULL)
    {
        *head = new_node;
    }
    else
    {
        struct Node* current = *head;
        while (current->next != NULL)
        {
            current = current->next;
        }
        current->next = new_node;
    }
}

// Функция для освобождения памяти списка
void free_list(struct Node* head)
{
    struct Node* current = head;
    while (current != NULL)
    {
        struct Node* next = current->next;
        free(current->data);
        free(current);
        current = next;
    }
}

// Функция для вывода списка
void print_list(struct Node* head)
{
    struct Node* current = head;
    while (current != NULL)
    {
        printf("%s", current->data);
        current = current->next;
    }
}

int main(){
    struct Node* head = NULL;
    char buffer[1024]; // Буфер для ввода строк
    
    printf("Вводите строки (точка в начале строки для завершения):\n");
    
    while (1)
    {
        // Читаем строку
        if (fgets(buffer, sizeof(buffer), stdin) == NULL)
        {
            break;
        }
        
        // Проверяем, не введена ли точка в начале строки
        if (buffer[0] == '.')
        {
            break;
        }
        
        // Создаем новый узел
        struct Node* new_node = create_node(buffer);
        if (new_node == NULL)
        {
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