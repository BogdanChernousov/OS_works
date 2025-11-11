#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

int set_mandatory_lock(int fd)
{
    struct stat st;
    if (fstat(fd, &st) == -1)
    {
        perror("fstat");
        return -1;
    }
    // Устанавливаем setgid и убираем group-execute для mandatory locking
    if (fchmod(fd, (st.st_mode & ~S_IXGRP) | S_ISGID) == -1)
    {
        perror("fchmod");
        return -1;
    }
    return 0;
}

int set_advisory_lock(int fd, int lock_type) {
    struct flock lock;
    lock.l_type = lock_type;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;

    if (fcntl(fd, F_SETLK, &lock) == -1) {
        if (errno == EAGAIN || errno == EACCES) {
            printf("Файл уже заблокирован другим процессом!\n");
        } else {
            perror("fcntl F_SETLK");
        }
        return -1;
    }
    return 0;
}

int call_editor(const char *filename) {
    const char *editor = getenv("VISUAL");
    if (!editor) editor = getenv("EDITOR");
    if (!editor) editor = "nano";

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return -1;
    } else if (pid == 0) {
        execlp(editor, editor, filename, (char *)NULL);
        perror("execlp");
        exit(1);
    } else {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            fprintf(stderr, "Редактор завершился с ошибкой\n");
            return -1;
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Использование: %s <файл>\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    printf("Текущий пользователь: %d\n", getuid());

    int fd = open(filename, O_RDWR);
    if (fd == -1) {
        perror("open failed");
        return 1;
    }

    // Устанавливаем mandatory lock
    if (set_mandatory_lock(fd) == -1) {
        close(fd);
        return 1;
    }

    printf("Устанавливаем advisory lock...\n");
    if (set_advisory_lock(fd, F_WRLCK) == -1) {
        close(fd);
        return 1;
    }

    printf("Блокировка успешно установлена! Файл защищен.\n");
    printf("Запускаем редактор...\n");

    if (call_editor(filename) == -1) {
        close(fd);
        return 1;
    }

    printf("Снимаем блокировку...\n");

    struct flock unlock;
    unlock.l_type = F_UNLCK;
    unlock.l_whence = SEEK_SET;
    unlock.l_start = 0;
    unlock.l_len = 0;

    if (fcntl(fd, F_SETLK, &unlock) == -1) {
        perror("fcntl F_UNLCK");
        close(fd);
        return 1;
    }

    close(fd);
    printf("Блокировка снята. Программа завершена.\n");
    return 0;
}