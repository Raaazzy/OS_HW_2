#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <semaphore.h>
#include <errno.h>
#include <sys/wait.h>
#include <time.h>

#define MAX_COUNT_CLIENTS 100
#define MAX_COUNT_PAINTS 5
sem_t* paints[MAX_COUNT_PAINTS];
int clients;
pid_t pid;

int main(int argc, char *argv[]) {
    sleep(1);
    if (argc != 3) {
        printf("Error arguments\n");
        return 1;
    }
    FILE *in_file = fopen(argv[1], "r");
    FILE *out_file = fopen(argv[2], "w");
    if (!in_file || !out_file) {
        printf("Error opening files\n");
        return 1;
    }
    fscanf(in_file, "%d", &clients);
    printf("Number of clients: %d\nNumber of paints: %d\n", clients, MAX_COUNT_PAINTS);
    // Открываем именованный семафор для доступа к количеству посетителей в галерее
    sem_t* sem_gallery = sem_open("/gallery", 0);
    paints[0] = sem_open("/first", 0);
    paints[1] = sem_open("/second", 0);
    paints[2] = sem_open("/third", 0);
    paints[3] = sem_open("/fourth", 0);
    paints[4] = sem_open("/fifth", 0);
    for (int i = 1; i <= clients; ++i) {
        pid = fork();
        // Дочерний процесс
        if (pid < 0) {
            perror("fork error");
            sem_close(sem_gallery);
            exit(-1);
            // Родительский процесс
        } else if (pid == 0) {
            // Выполняем блокировку семафора
            sem_wait(sem_gallery);
            printf("Client %d come in \n", i);
            fprintf(out_file, "Client %d come in \n", i);
            srand(getpid());
            int watched[MAX_COUNT_PAINTS] = {0, 0, 0, 0, 0};
            for (int i = 0; i < MAX_COUNT_PAINTS; ++i) {
                if (watched[0] == 1 && watched[1] == 1 && watched[2] == 1 && watched[3] == 1 && watched[4] == 1) {
                    break;
                }
                int j = rand() % MAX_COUNT_PAINTS;
                while (watched[j]) {
                    j = rand() % MAX_COUNT_PAINTS;
                }
                watched[j] = 1;
                // Выполняем блокировку семафора
                sem_wait(paints[j]);
                printf("New client %d is watching paint number %d\n", i, j);
                fprintf(out_file, "New client %d is watching paint number %d\n", i, j);
                sleep(rand() % MAX_COUNT_PAINTS);
                printf("Client %d finished watching the paint number %d\n", i, j);
                fprintf(out_file, "Client %d finished watching the paint number %d\n", i, j);
                sem_post(paints[j]);
            }
            // Освобождаем семафор
            printf("Client %d left\n", i);
            fprintf(out_file, "Client %d left\n", i);
            sem_post(sem_gallery);
            exit(0);
        }
    }
    // Родительский процесс ожидает заверешния работы всех процессов
    if (pid > 0) {
        while (pid = waitpid(-1, NULL, 0)) {
            if (errno == ECHILD) {
                break;
            }
        }
        sem_post(sem_gallery);
        exit(0);
    }
}

