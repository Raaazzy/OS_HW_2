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
    // Удаляем неименованный семафор
    sem_destroy(sem_gallery);
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
    sem_gallery = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (sem_gallery == MAP_FAILED) {
        printf("Error mapping semaphore memory\n");
        return 1;
    }
    // Создаем неименованный семафор
    if (sem_init(sem_gallery, 1, MAX_COUNT_CLIENTS) == -1) {
        printf("Error initializing semaphore\n");
        return 1;
    }
    // Созданием объект разделяемой памяти (файловый дескриптор)
    int fd = shm_open("/gallery_shm", O_CREAT | O_EXCL | O_RDWR, 0666);
    if (fd == -1) {
        printf("Error creating shared memory\n");
        return 1;
    }
    if (ftruncate(fd, SIZE) == -1) {
        printf("Error setting shared memory size\n");
        return 1;
    }
    // Созданием область разделяемой памяти и связываем с файловым дескриптором
    shm_gallery = mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm_gallery == MAP_FAILED) {
        printf("Error mapping shared memory\n");
        return 1;
    }
    close(fd);
    for (int i = 1; i <= paints; i++) {
        shm_gallery[i] = 0;
    }
    for (int i = 1; i <= clients; i++) {
        pid = fork();
        // Дочерний процесс
        if (pid < 0) {
            printf("Error creating child process\n");
            return 1;
            // Родительский процесс
        } else if (pid == 0) {
            for (int j = 1; j <= paints; j++) {
                // Приостановливаем процесс
                usleep(rand() % 1000);
                // Выполняем блокировку семафора
                sem_wait(sem_gallery);
                if (shm_gallery[j] >= MAX_COUNT_VIEWERS) {
                    printf("Client %d is waiting for paint %d\n", i, j);
                    fprintf(out_file, "Client %d is waiting for paint %d\n", i, j);
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
    // Удаляем неименованный семафор
    sem_destroy(sem_gallery);
    // Отключаем отображение разделяемой памяти
    munmap(shm_gallery, SIZE);
    //Закрытие файлов
    fclose(in_file);
    fclose(out_file);
    return 0;
}
