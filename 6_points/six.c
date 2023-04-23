#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <signal.h>

#define MAX_COUNT_CLIENTS 100
#define MAX_COUNT_PAINTS 100
#define MAX_COUNT_VIEWERS 10
#define SEM_KEY 1234
#define SHM_KEY 5678
#define SIZE sizeof(int[MAX_COUNT_PAINTS])
int paints;
int clients;
int *shm_gallery;
int sem_id;
int shm_id;
pid_t pid;

void handler(int signum) {
    // Удаляем семафор с идентификатором sem_id
    semctl(sem_id, 0, IPC_RMID, 0);
    // Отсоединяем общую память от адресного пространства текущего процесса
    shmdt(shm_gallery);
    // Удалением разделяемую память с идентификатором shm_id
    shmctl(shm_id, IPC_RMID, 0);
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
    shm_id = shmget(SHM_KEY, SIZE, IPC_CREAT | 0666);
    if (shm_id == -1) {
        printf("Error creating shared memory\n");
        return 1;
    }
    // Подключаем область разделяемой памяти к адресному пространству текущего процесса
    shm_gallery = shmat(shm_id, NULL, 0);
    if (shm_gallery == (int *) -1) {
        printf("Error attaching shared memory\n");
        return 1;
    }
    // Получаем id существующего или создаем новый семафор
    sem_id = semget(SEM_KEY, MAX_COUNT_PAINTS, IPC_CREAT | 0666);
    if (sem_id == -1) {
        printf("Error creating semaphore set\n");
        return 1;
    }
    for (int i = 1; i <= paints; i++) {
        shm_gallery[i] = 0;
        semctl(sem_id, i, SETVAL, MAX_COUNT_CLIENTS);
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
                if (shm_gallery[j] >= MAX_COUNT_VIEWERS) {
                    printf("Client %d is waiting for paint %d\n", i, j);
                    fprintf(out_file, "Client %d is waiting for paint %d\n", i, j);
                    usleep(rand() % 100000);
                }
                shm_gallery[j]++;
                printf("Client %d watched paint %d with %d viewers\n", i, j, shm_gallery[j]);
                fprintf(out_file, "Client %d watched paint %d with %d viewers\n", i, j, shm_gallery[j]);
            }
            exit(0);
        }
    }

    // Завершаем дочерние процессы
    for (int i = 1; i <= clients; i++) {
        wait(NULL);
    }
    // Удаляем семафор с идентификатором sem_id
    semctl(sem_id, 0, IPC_RMID, 0);
    // Отсоединяем общую память от адресного пространства текущего процесса
    shmdt(shm_gallery);
    // Удалением разделяемую память с идентификатором shm_id
    shmctl(shm_id, IPC_RMID, 0);
    fclose(in_file);
    fclose(out_file);
    return 0;
}
