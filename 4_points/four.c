#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <signal.h>

#define MAX_COUNT_CLIENTS 100
#define MAX_COUNT_PAINTS 100
#define MAX_COUNT_VIEWERS 10
#define SIZE sizeof(int[MAX_COUNT_PAINTS])
int paints;
int clients;
sem_t *sem_gallery;
int *shm_gallery;
pid_t pid;

void handler(int signum) {
    // Закрываем именованный семафор
    sem_close(sem_gallery);
    // Удаляем именованный семафор
    sem_unlink("/gallery_sem");
    // Отключаем отображение разделяемой памяти
    munmap(shm_gallery, SIZE);
    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Error arguments\n");
        return 1;
    }
    // Устанавливаем обработчик на сигнал SIGINT при нажатии Ctrl C
    signal(SIGINT, handler);
    FILE *in_file = fopen(argv[1], "r");
    FILE *out_file = fopen(argv[2], "w");
    if (!in_file || !out_file) {
        printf("Error opening files\n");
        return 1;
    }
    fscanf(in_file, "%d %d", &clients, &paints);
    printf("Number of clients: %d\nNumber of paints: %d\n", clients, paints);
    fprintf(out_file, "Number of clients: %d\nNumber of paints: %d\n", clients, paints);
    // Создаем область разделяемой памяти
    shm_gallery = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (shm_gallery == MAP_FAILED) {
        printf("Error mapping shared memory\n");
        return 1;
    }
    // Открываем именованный семафор
    sem_gallery = sem_open("/gallery_sem", O_CREAT | O_EXCL, 0666, MAX_COUNT_CLIENTS);
    if (sem_gallery == SEM_FAILED) {
        printf("Error creating semaphore\n");
        return 1;
    }
    for (int i = 1; i <= paints; i++) {
        shm_gallery[i] = 0;
    }
    for (int i = 1; i <= clients; i++) {
        pid = fork();
        // Дочерний процесс
        if (pid < 0) {
            printf("Error child process\n");
            return 1;
            // Родительский процесс
        } else if (pid == 0) {
            for (int j = 1; j <= paints; j++) {
                // Приостановливаем процесс
                usleep(rand() % 1000);
                // Выполняем блокировку семафора
                sem_wait(sem_gallery);
                if (shm_gallery[j] >= MAX_COUNT_VIEWERS) {
                    printf("Client %d is waiting %d\n", i, j);
                    fprintf(out_file, "Client %d is waiting %d\n", i, j);
                    sem_wait(sem_gallery);
                }
                shm_gallery[j]++;
                printf("Client %d watched paint %d with %d viewers\n", i, j, shm_gallery[j]);
                fprintf(out_file, "Client %d watched paint %d with %d viewers\n", i, j, shm_gallery[j]);
                // Освобождаем семафор
                sem_post(sem_gallery);
            }
            exit(0);
        }
    }
    // Завершаем дочерние процессы
    for (int i = 1; i <= clients; i++) {
        wait(NULL);
    }
    // Закрываем именованный семафор
    sem_close(sem_gallery);
    // Удаляем именованный семафор
    sem_unlink("/gallery_sem");
    // Отключаем отображение разделяемой памяти
    munmap(shm_gallery, SIZE);
    fclose(in_file);
    fclose(out_file);
    return 0;
}
