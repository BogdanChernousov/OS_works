#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <ulimit.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <getopt.h>
#include <linux/limits.h>

// внешняя переменная - окружение процесса
extern char** environ;

// печать реального и эффективного идентификатора пользователя
void print_user_ids()
{
    printf("The real user ID: %d.\n", getuid());
    printf("The effective user ID: %d.\n", geteuid());
}

// печать реального и эффективного идентификатора группы
void print_group_ids()
{
    printf("The real group ID: %d.\n", getgid());
    printf("The effective group ID: %d.\n", getegid());
}

// печать идентификатора процесса
void print_proc_id()
{
    pid_t proc_id = getpid();
    printf("The process ID: %d.\n", proc_id);
}

// печать идентификатора родительского процесса
void print_parent_proc_id()
{
    pid_t parent_id = getppid();
    printf("The parent process ID: %d.\n", parent_id);
}

// печать идентификатора группы процессов
void print_group_proc_id()
{
    pid_t group_id = getpgid(0);
    printf("The group process ID: %d.\n", group_id);
}

// печать значения ulimit
void print_u_limit()
{
    struct rlimit rlim;
    
    if (getrlimit(RLIMIT_NPROC, &rlim) == -1)
    {
        perror("Error while getting process limit");
    }
    else
    {
        printf("Max user processes (-u): %ld\n", (long)rlim.rlim_cur);
    }
}

// установка нового значения максимального количества процессов
void set_u_limit(long new_limit)
{
    struct rlimit rlim;
    
    // Сначала получаем текущие лимиты
    if (getrlimit(RLIMIT_NPROC, &rlim) == -1)
    {
        perror("Error while getting current process limit");
        return;
    }
    
    // Устанавливаем новое мягкое ограничение
    rlim.rlim_cur = new_limit;
    
    if (setrlimit(RLIMIT_NPROC, &rlim) == -1)
    {
        perror("Error while setting process limit");
    }
    else
    {
        printf("Success! New max user processes: %ld\n", new_limit);
    }
}

// печать размера core-файла
void print_core_limit()
{
    struct rlimit rlim;
    int error = getrlimit(RLIMIT_CORE, &rlim);
    if (error == -1 && errno != 0)
    {
        perror("Error while getting RLIMIT_CORE");
    }
    else
    {
        printf("Cur core limit is %lld.\n", (long long)rlim.rlim_cur);
        printf("Max core limit is %lld.\n", (long long)rlim.rlim_max);
    }
}

// установка нового размера core-файла
void set_core_limit(long new_core_size)
{
    struct rlimit rlim;
    if (getrlimit(RLIMIT_CORE, &rlim) == -1) {
        perror("Error getting current RLIMIT_CORE");
        return;
    }
    rlim.rlim_cur = new_core_size;
    
    int error = setrlimit(RLIMIT_CORE, &rlim);
    if (error == -1)
    {
        perror("Error while setting RLIMIT_CORE");
    }
    else
    {
        printf("Success! Core limit is %ld.\n", new_core_size);
    }
}

// печать текущей рабочей директории
void print_directory()
{
    char buffer[PATH_MAX + 1] = {0};
    char* error = getcwd(buffer, PATH_MAX);

    if (error == NULL)
    {
        perror("Error getting directory");
    }
    else
    {
        printf("Current work directory is '%s'.\n", buffer);
    }
}

// печать переменных среды
void print_env_vars()
{
    printf("Environment variables:\n");
    for (int i = 0; environ[i] != NULL; i++)
    {
        printf("\t%s\n", environ[i]);
    }
}

// добавление или изменение переменной среды
void new_or_change_env_var(char* str)
{
    int error = putenv(str);
    if (error == 0)
    {
        printf("putenv() was successful.\n");
    }
    else
    {
        perror("Error");
    }
}

// переворачивает аргументы для обработки справа налево
void reverse_argv(int argc, char* argv[])
{
    for (int i = 1, j = argc - 1; i < j; i++, j--) {
        char* temp = argv[i];
        argv[i] = argv[j];
        argv[j] = temp;
    }
}

int main(int argc, char* argv[])
{
    if (argc <= 1)
    {
        printf("No arguments.\n");
        return 0;
    }
    
    // переворачиваем аргументы для обработки справа налево
    reverse_argv(argc, argv);
    
    // строка опций: буква - без аргумента, буква: - с обязательным аргументом
    char options[] = "ispuU:cC:dvV:";
    int c = 0;
    int arg_cnt = 0;

    long new_u_limit = 0;
    long new_core_size = 0;
    char* endptr;
    
    // обработка опций с помощью getopt
    while ((c = getopt(argc, argv, options)) != EOF)
    {
        arg_cnt++;
        switch (c)
        {
            case 'i':
                print_user_ids();
                print_group_ids();
                break;

            case 's':
                // процесс становится лидером группы
                setpgid(0, 0);
                break;

            case 'p':
                print_proc_id();
                print_parent_proc_id();
                print_group_proc_id();
                break;

            case 'u':
                print_u_limit();
                break;

            case 'U':
                // преобразуем строку в число для ulimit
                new_u_limit = strtol(optarg, &endptr, 10);
                if (endptr != optarg)
                {
                    set_u_limit(new_u_limit);
                }
                else
                {
                    printf("Argument '%s' isn't number.\n", optarg);
                }
                break;

            case 'c':
                print_core_limit();
                break;

            case 'C':
                // преобразуем строку в число для core limit
                new_core_size = strtol(optarg, &endptr, 10);
                if (endptr != optarg)
                {
                    set_core_limit(new_core_size);
                }
                else
                {
                    printf("Argument '%s' isn't number.\n", optarg);
                }
                break;

            case 'd':
                print_directory();
                break;

            case 'v':
                print_env_vars();
                break;

            case 'V':
                new_or_change_env_var(optarg);
                break;
                
            case '?':
                printf("Invalid option: '%c'\n", optopt);
                break;
        }
    }

    if (arg_cnt == 0)
    {
        printf("No valid options.\n");
    }

    return 0;
}